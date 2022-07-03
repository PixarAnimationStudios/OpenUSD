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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgiGL/sampler.h"
#include "pxr/imaging/hgiGL/shaderProgram.h"

#include "pxr/imaging/hgi/resourceBindings.h"

#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdSt_ResourceBindingSuffixTokens,
                        HDST_RESOURCE_BINDING_SUFFIX_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_double, "double"))
    ((_float, "float"))
    ((_int, "int"))
    (vec2)
    (vec3)
    (vec4)
    (dvec2)
    (dvec3)
    (dvec4)
    (ivec2)
    (ivec3)
    (ivec4)
    (constantPrimvars)
    (primitiveParam)
    (topologyVisibility)
);

namespace {
    struct BindingLocator {
        BindingLocator() :
            uniformLocation(0),
            bufferLocation(0),
            attribLocation(0),
            textureUnit(0) {}

        HdBinding GetBinding(HdBinding::Type type, TfToken const &debugName) {
            switch(type) {
            case HdBinding::UNIFORM:
                return HdBinding(HdBinding::UNIFORM, uniformLocation++);
                break;
            case HdBinding::UBO:
                return HdBinding(HdBinding::UBO, bufferLocation++);
                break;
            case HdBinding::SSBO:
                return HdBinding(HdBinding::SSBO, bufferLocation++);
                break;
            case HdBinding::BINDLESS_SSBO_RANGE:
                return HdBinding(HdBinding::BINDLESS_SSBO_RANGE, uniformLocation++);
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
        int bufferLocation;
        int attribLocation;
        int textureUnit;
    };

    static inline GLboolean _ShouldBeNormalized(HdType type) {
        return type == HdTypeInt32_2_10_10_10_REV;
    }

    // GL has special handling for the "number of components" for
    // packed vectors.  Handle that here.
    static inline int _GetNumComponents(HdType type) {
        if (type == HdTypeInt32_2_10_10_10_REV) {
            return 4;
        } else {
            return HdGetComponentCount(type);
        }
    }

    // Modify datatype if swizzle is specified
    static HdType _AdjustHdType(HdType type, std::string const &swizzle) {
        size_t numChannels = swizzle.size();
        if (numChannels == 4) {
            return HdTypeFloatVec4;
        } else if (numChannels == 3) {
            return HdTypeFloatVec3;
        } else if (numChannels == 2) {
            return HdTypeFloatVec2;
        } else if (numChannels == 1) {
            return HdTypeFloat;
        }
        
        return type;
    }
}

HdSt_ResourceBinder::HdSt_ResourceBinder()
{
}

static
TfToken
_ConcatLayout(const TfToken &token)
{
    return TfToken(
        token.GetString()
        + HdSt_ResourceBindingSuffixTokens->layout.GetString());
}

static
TfTokenVector
_GetInstancerFilterNames(HdStDrawItem const * drawItem)
{
    TfTokenVector filterNames = HdInstancer::GetBuiltinPrimvarNames();;

    HdSt_MaterialNetworkShaderSharedPtr materialNetworkShader =
        drawItem->GetMaterialNetworkShader();
    if (materialNetworkShader) {
        TfTokenVector const & names = materialNetworkShader->GetPrimvarNames();
        filterNames.insert(filterNames.end(), names.begin(), names.end());
    }

    return filterNames;
}

static
bool
_TokenContainsString(const TfToken &token, const std::string &string)
{
    return (token.GetString().find(string) != std::string::npos);
}

void
HdSt_ResourceBinder::ResolveBindings(HdStDrawItem const *drawItem,
                                   HdStShaderCodeSharedPtrVector const &shaders,
                                   HdSt_ResourceBinder::MetaData *metaDataOut,
                                   bool instanceDraw,
                                   HdBindingRequestVector const &customBindings,
                                   HgiCapabilities const *capabilities)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(metaDataOut)) return;

    const bool bindlessBuffersEnabled = 
        capabilities->IsSet(HgiDeviceCapabilitiesBitsBindlessBuffers);
    const bool bindlessTexturesEnabled = 
        capabilities->IsSet(HgiDeviceCapabilitiesBitsBindlessTextures);

    HdBinding::Type arrayBufferBindingType = HdBinding::SSBO;
    if (bindlessBuffersEnabled) {
        arrayBufferBindingType = HdBinding::BINDLESS_UNIFORM; // EXT
    }

    HdBinding::Type structBufferBindingType = HdBinding::UBO;  // 3.1
    if (bindlessBuffersEnabled) {
        structBufferBindingType = HdBinding::BINDLESS_UNIFORM; // EXT
    } else {
        structBufferBindingType = HdBinding::SSBO;             // 4.3
    }

    HdBinding::Type drawingCoordBindingType =
        instanceDraw
            ? HdBinding::DRAW_INDEX_INSTANCE
            : HdBinding::DRAW_INDEX;

    // binding assignments
    BindingLocator locator;

    int bindlessTextureLocation = 0;
    // Note that these locations are used for hash keys only and
    // are never used for actual resource binding.
    int shaderFallbackLocation = 0;
    int shaderPrimvarRedirectLocation = 0;
    int shaderFieldRedirectLocation = 0;
    int shaderTransform2dLocation = 0;

    // clear all
    _bindingMap.clear();

    // constant primvar (per-object)
    HdBinding constantPrimvarBinding =
                locator.GetBinding(structBufferBindingType,
                                   _tokens->constantPrimvars);

    if (HdBufferArrayRangeSharedPtr constantBar_ =
        drawItem->GetConstantPrimvarRange()) {

        HdStBufferArrayRangeSharedPtr constantBar =
            std::static_pointer_cast<HdStBufferArrayRange>(constantBar_);

        MetaData::StructBlock sblock(_tokens->constantPrimvars);
        TF_FOR_ALL (it, constantBar->GetResources()) {
            HdTupleType valueType = it->second->GetTupleType();
            TfToken glType = HdStGLConversions::GetGLSLTypename(valueType.type);
            TfToken glName =  HdStGLConversions::GetGLSLIdentifier(it->first);
            sblock.entries.emplace_back(
                /*name=*/glName,
                /*type=*/glType,
                /*offset=*/it->second->GetOffset(),
                /*arraySize=*/valueType.count);
        }
        // sort by offset
        // XXX: not robust enough, should consider padding and layouting rules
        // to match with the logic in HdInterleavedMemoryManager if we
        // want to use a layouting policy other than default padding.
        std::sort(sblock.entries.begin(), sblock.entries.end());

        metaDataOut->constantData.insert(
            std::make_pair(constantPrimvarBinding, sblock));
    }

     // constant primvars are interleaved into single struct.
    _bindingMap[_tokens->constantPrimvars] = constantPrimvarBinding;

    TfTokenVector filterNames = _GetInstancerFilterNames(drawItem);

    // instance primvar (per-instance)
    int instancerNumLevels = drawItem->GetInstancePrimvarNumLevels();
    metaDataOut->instancerNumLevels = instancerNumLevels;
    for (int i = 0; i < instancerNumLevels; ++i) {
        if (HdBufferArrayRangeSharedPtr instanceBar_ =
            drawItem->GetInstancePrimvarRange(i)) {

            HdStBufferArrayRangeSharedPtr instanceBar =
                std::static_pointer_cast<HdStBufferArrayRange>(instanceBar_);

            TF_FOR_ALL (it, instanceBar->GetResources()) {
                TfToken const& name = it->first;
                // skip instance primvars that are not used in this batch.
                if (std::find(filterNames.begin(), filterNames.end(), name)
                                                        == filterNames.end()) {
                    continue;
                }

                TfToken glName =  HdStGLConversions::GetGLSLIdentifier(name);
                // non-interleaved, always create new binding.
                HdBinding instancePrimvarBinding =
                    locator.GetBinding(arrayBufferBindingType, name);
                _bindingMap[NameAndLevel(name, i)] = instancePrimvarBinding;

                HdTupleType valueType = it->second->GetTupleType();
                TfToken glType =
                    HdStGLConversions::GetGLSLTypename(valueType.type);
                metaDataOut->instanceData[instancePrimvarBinding] =
                    MetaData::NestedPrimvar(
                        /*name=*/glName,
                        /*type=*/glType,
                        /*level=*/i);
            }
        }
    }

    // vertex primvar (per-vertex)
    // always assigned to VertexAttribute.
    if (HdBufferArrayRangeSharedPtr vertexBar_ =
        drawItem->GetVertexPrimvarRange()) {

        HdStBufferArrayRangeSharedPtr vertexBar =
            std::static_pointer_cast<HdStBufferArrayRange>(vertexBar_);

        TF_FOR_ALL (it, vertexBar->GetResources()) {
            TfToken const& name = it->first;
            TfToken glName =  HdStGLConversions::GetGLSLIdentifier(name);
            HdBinding vertexPrimvarBinding =
                locator.GetBinding(HdBinding::VERTEX_ATTR, name);
            _bindingMap[name] = vertexPrimvarBinding;

            HdTupleType valueType = it->second->GetTupleType();
            // Special case: VBOs have intrinsic support for packed types,
            // so expand them out to their target type for the shader binding.
            if (valueType.type == HdTypeInt32_2_10_10_10_REV) {
                valueType.type = HdTypeFloatVec4;
            } else if (valueType.type == HdTypeHalfFloatVec2) {
                valueType.type = HdTypeFloatVec2;
            } else if (valueType.type == HdTypeHalfFloatVec4) {
                valueType.type = HdTypeFloatVec4;
            }
            TfToken glType = HdStGLConversions::GetGLSLTypename(valueType.type);
            metaDataOut->vertexData[vertexPrimvarBinding] =
                MetaData::Primvar(/*name=*/glName,
                                  /*type=*/glType);
        }
    }

    // varying primvar
    if (HdBufferArrayRangeSharedPtr varyingBar_ =
        drawItem->GetVaryingPrimvarRange()) {

        HdStBufferArrayRangeSharedPtr varyingBar =
            std::static_pointer_cast<HdStBufferArrayRange>(varyingBar_);

        for (const auto &resource : varyingBar->GetResources()) {
            TfToken const& name = resource.first;
            TfToken glName =  HdStGLConversions::GetGLSLIdentifier(name);
            HdBinding varyingPrimvarBinding =
                locator.GetBinding(arrayBufferBindingType, name);
            _bindingMap[name] = varyingPrimvarBinding;

            HdTupleType valueType = resource.second->GetTupleType();
            TfToken glType = HdStGLConversions::GetGLSLTypename(valueType.type);
            metaDataOut->varyingData[varyingPrimvarBinding] =
                MetaData::Primvar(/*name=*/glName,
                                  /*type=*/glType);
        }
    }

    // index buffer
    if (HdBufferArrayRangeSharedPtr topologyBar_ =
        drawItem->GetTopologyRange()) {

        HdStBufferArrayRangeSharedPtr topologyBar =
            std::static_pointer_cast<HdStBufferArrayRange>(topologyBar_);

        TF_FOR_ALL (it, topologyBar->GetResources()) {
            // Don't need to sanitize the name, since topology resources are
            // created internally.
            TfToken const& name = it->first;
            HdStBufferResourceSharedPtr const& resource = it->second;

            if (name == HdTokens->indices) {
                // IBO. no need for codegen
                _bindingMap[name] = HdBinding(HdBinding::INDEX_ATTR, 0);
            } else {
                // We expect the following additional topology based info:
                // - primitive parameter (for all tris, quads and patches)
                // - edge indices (for all tris, quads and patches)
                // - fvar indices (for refined tris, quads, and patches with
                //   face-varying primvars)
                // - fvar patch params (for refined tris, quads, and patches 
                //   with face-varying primvars)

                HdBinding binding =
                    locator.GetBinding(arrayBufferBindingType, name);
                _bindingMap[name] = binding;

                HdTupleType valueType = resource->GetTupleType();
                TfToken glType =
                    HdStGLConversions::GetGLSLTypename(valueType.type);

                auto bindingDecl = MetaData::BindingDeclaration(
                                     /*name=*/name,
                                     /*type=*/glType,
                                     /*binding=*/binding);

                if (name == HdTokens->primitiveParam) {
                    metaDataOut->primitiveParamBinding = bindingDecl;
                } else if (name == HdTokens->edgeIndices) {
                    metaDataOut->edgeIndexBinding = bindingDecl;
                } else if (name == HdStTokens->coarseFaceIndex) {
                    metaDataOut->coarseFaceIndexBinding = bindingDecl;
                } else if (_TokenContainsString(name,
                           HdStTokens->fvarIndices.GetString())) {
                    metaDataOut->fvarIndicesBindings.push_back(bindingDecl);
                } else if (_TokenContainsString(name, 
                           HdStTokens->fvarPatchParam.GetString())) {
                    metaDataOut->fvarPatchParamBindings.push_back(bindingDecl);
                } else {
                    TF_WARN("Unexpected topological resource '%s'\n",
                    name.GetText());
                }
            }
        }
    }

     // topology visibility
    HdBinding topologyVisibilityBinding =
                locator.GetBinding(structBufferBindingType,
                                   /*debugName*/_tokens->topologyVisibility);

    if (HdBufferArrayRangeSharedPtr topVisBar_ =
        drawItem->GetTopologyVisibilityRange()) {

        HdStBufferArrayRangeSharedPtr topVisBar =
            std::static_pointer_cast<HdStBufferArrayRange>(topVisBar_);

        MetaData::StructBlock sblock(_tokens->topologyVisibility);
        TF_FOR_ALL (it, topVisBar->GetResources()) {
            HdTupleType valueType = it->second->GetTupleType();
            TfToken glType = HdStGLConversions::GetGLSLTypename(valueType.type);
            sblock.entries.emplace_back(
                /*name=*/it->first,
                /*type=*/glType,
                /*offset=*/it->second->GetOffset(),
                /*arraySize=*/valueType.count);
        }
        
        std::sort(sblock.entries.begin(), sblock.entries.end());

        metaDataOut->topologyVisibilityData.insert(
            std::make_pair(topologyVisibilityBinding, sblock));
    }

     // topology visibility is interleaved into single struct.
    _bindingMap[_tokens->topologyVisibility] = topologyVisibilityBinding;

    // element primvar (per-face, per-line)
    if (HdBufferArrayRangeSharedPtr elementBar_ =
        drawItem->GetElementPrimvarRange()) {

        HdStBufferArrayRangeSharedPtr elementBar =
            std::static_pointer_cast<HdStBufferArrayRange>(elementBar_);

        TF_FOR_ALL (it, elementBar->GetResources()) {
            TfToken const& name = it->first;
            TfToken glName =  HdStGLConversions::GetGLSLIdentifier(name);
            HdBinding elementPrimvarBinding =
                locator.GetBinding(arrayBufferBindingType, name);
            _bindingMap[name] = elementPrimvarBinding;
            HdTupleType valueType = it->second->GetTupleType();
                TfToken glType =
                    HdStGLConversions::GetGLSLTypename(valueType.type);
            metaDataOut->elementData[elementPrimvarBinding] =
                MetaData::Primvar(/*name=*/glName,
                                  /*type=*/glType);
        }
    }

    // facevarying primvar (per-face-vertex)
    if (HdBufferArrayRangeSharedPtr fvarBar_ =
        drawItem->GetFaceVaryingPrimvarRange()) {

        HdStBufferArrayRangeSharedPtr fvarBar =
            std::static_pointer_cast<HdStBufferArrayRange>(fvarBar_);

        TopologyToPrimvarVector const & fvarTopoToPvMap = 
            drawItem->GetFvarTopologyToPrimvarVector();
        
        TF_FOR_ALL (it, fvarBar->GetResources()) {
            TfToken const& name = it->first;
            TfToken glName =  HdStGLConversions::GetGLSLIdentifier(name);

            HdBinding fvarPrimvarBinding =
                locator.GetBinding(arrayBufferBindingType, name);
            _bindingMap[name] = fvarPrimvarBinding;
            HdTupleType valueType = it->second->GetTupleType();
            TfToken glType = HdStGLConversions::GetGLSLTypename(valueType.type);

            // Fine if no channel is found, might be unrefined primvar
            int fvarChannel = 0;
            for (size_t i = 0; i < fvarTopoToPvMap.size(); ++i) {
                if (std::find(fvarTopoToPvMap[i].second.begin(), 
                              fvarTopoToPvMap[i].second.end(),
                              name) != fvarTopoToPvMap[i].second.end()) {
                    fvarChannel = i;
                }
            }
            
            metaDataOut->fvarData[fvarPrimvarBinding] =
                MetaData::FvarPrimvar(/*name=*/glName,
                                      /*type=*/glType,
                                      /*channel=*/fvarChannel);
        }
    }

    // draw parameter
    // assigned to draw index (vertex attributeI w/divisor) (indirect)
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
                                     /*type=*/_tokens->ivec4,
                                     /*binding=*/drawingCoord1Binding);

    HdBinding drawingCoord2Binding = locator.GetBinding(
        drawingCoordBindingType, HdTokens->drawingCoord2);
    _bindingMap[HdTokens->drawingCoord2] = drawingCoord2Binding;
    metaDataOut->drawingCoord2Binding =
        MetaData::BindingDeclaration(/*name=*/HdTokens->drawingCoord2,
                                     /*type=*/_tokens->ivec2,
                                     /*binding=*/drawingCoord2Binding);

    if (instancerNumLevels > 0) {
        HdBinding drawingCoordIBinding =
            HdBinding(HdBinding::DRAW_INDEX_INSTANCE_ARRAY,
                      locator.attribLocation);

        // each vertex attribute takes 1 location
        locator.attribLocation += instancerNumLevels;

        _bindingMap[HdTokens->drawingCoordI] = drawingCoordIBinding;
        metaDataOut->drawingCoordIBinding =
            MetaData::BindingDeclaration(/*name=*/HdTokens->drawingCoordI,
                                         /*type=*/_tokens->_int,
                                         /*binding=*/drawingCoordIBinding);
    }

    // instance index indirection buffer
    if (HdBufferArrayRangeSharedPtr instanceIndexBar_ =
        drawItem->GetInstanceIndexRange()) {

        HdStBufferArrayRangeSharedPtr instanceIndexBar =
            std::static_pointer_cast<HdStBufferArrayRange>(
                                                        instanceIndexBar_);

        HdStBufferResourceSharedPtr instanceIndices
            = instanceIndexBar->GetResource(
                                    HdInstancerTokens->instanceIndices);
        HdStBufferResourceSharedPtr culledInstanceIndices
            = instanceIndexBar->GetResource(
                                    HdInstancerTokens->culledInstanceIndices);

        if (instanceIndices) {
            HdBinding instanceIndexArrayBinding =
                locator.GetBinding(arrayBufferBindingType,
                                   HdInstancerTokens->instanceIndices);
            _bindingMap[HdInstancerTokens->instanceIndices] =
                instanceIndexArrayBinding;
            HdTupleType valueType = instanceIndices->GetTupleType();
                TfToken glType =
                    HdStGLConversions::GetGLSLTypename(valueType.type);
            metaDataOut->instanceIndexArrayBinding =
                MetaData::BindingDeclaration(
                    /*name=*/HdInstancerTokens->instanceIndices,
                    /*type=*/glType,
                    /*binding=*/instanceIndexArrayBinding);
        }
        if (culledInstanceIndices) {
            HdBinding culledInstanceIndexArrayBinding =
                locator.GetBinding(arrayBufferBindingType,
                                   HdInstancerTokens->culledInstanceIndices);
            _bindingMap[HdInstancerTokens->culledInstanceIndices] =
                culledInstanceIndexArrayBinding;
            HdTupleType valueType = instanceIndices->GetTupleType();
                TfToken glType =
                    HdStGLConversions::GetGLSLTypename(valueType.type);
            metaDataOut->culledInstanceIndexArrayBinding =
                MetaData::BindingDeclaration(
                    /*name=*/HdInstancerTokens->culledInstanceIndices,
                    /*type=*/glType,
                    /*binding=*/culledInstanceIndexArrayBinding,
                    /*isWritable=*/true);
        }
    }

    HdBinding dispatchBinding(HdBinding::DISPATCH, /*location=(not used)*/0);
    _bindingMap[HdTokens->drawDispatch] = dispatchBinding;

    // shader parameter bindings
    TF_FOR_ALL(shader, shaders) {

        // uniform block
        HdBufferArrayRangeSharedPtr const &shaderBar_ = 
                                                (*shader)->GetShaderData();
        HdStBufferArrayRangeSharedPtr shaderBar =
            std::static_pointer_cast<HdStBufferArrayRange> (shaderBar_);
        if (shaderBar) {
            HdBinding shaderParamBinding =
                locator.GetBinding(structBufferBindingType, 
                                    HdTokens->materialParams);

            // for fallback values and bindless textures
            // XXX: name of sblock must be unique for each shaders.
            MetaData::StructBlock sblock(HdTokens->materialParams);
            TF_FOR_ALL(it, shaderBar->GetResources()) {
                TfToken const& name = it->first;
                TfToken glName =  HdStGLConversions::GetGLSLIdentifier(name);
                HdTupleType valueType = it->second->GetTupleType();
                TfToken glType =
                    HdStGLConversions::GetGLSLTypename(valueType.type);
                sblock.entries.emplace_back(
                    /*name=*/glName,
                    /*type=*/glType,
                    /*offset=*/it->second->GetOffset(),
                    /*arraySize=*/valueType.count);
            }
            // sort by offset
            std::sort(sblock.entries.begin(), sblock.entries.end());
            metaDataOut->shaderData.insert(
                std::make_pair(shaderParamBinding, sblock));

            //XXX:hack  we want to generalize materialParams to other shaders.
            if ((*shader) == drawItem->GetMaterialNetworkShader()) {
                // shader parameters are interleaved into single struct.
                _bindingMap[HdTokens->materialParams] = shaderParamBinding;
            }
        }

        HdSt_MaterialParamVector params = (*shader)->GetParams();
        // for primvar and texture accessors
        for (HdSt_MaterialParam const& param : params) {
            const bool isMaterialShader =
                ((*shader) == drawItem->GetMaterialNetworkShader());

            // renderpass texture should be bindfull (for now)
            const bool bindless = bindlessTexturesEnabled && isMaterialShader;
            std::string const& glSwizzle = param.swizzle;                    
            HdTupleType valueType = param.GetTupleType();
            TfToken glType =
                HdStGLConversions::GetGLSLTypename(_AdjustHdType(valueType.type,
                                                                 glSwizzle));
            TfToken const& name = param.name;
            TfToken glName =  HdStGLConversions::GetGLSLIdentifier(name);

            if (param.IsFallback()) {
                metaDataOut->shaderParameterBinding[
                            HdBinding(HdBinding::FALLBACK, 
                            shaderFallbackLocation++)]
                    = MetaData::ShaderParameterAccessor(glName, 
                                                        /*type=*/glType);
            }
            else if (param.IsTexture()) {
                if (param.textureType == HdTextureType::Ptex) {
                    // ptex texture
                    HdBinding texelBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL,
                                    bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_PTEX_TEXEL,
                                    locator.uniformLocation++, 
                                    locator.textureUnit++);

                    metaDataOut->shaderParameterBinding[texelBinding] =
                        MetaData::ShaderParameterAccessor(
                            /*name=*/glName,
                            /*type=*/glType,
                            /*swizzle=*/glSwizzle,
                            /*inPrimvars=*/param.samplerCoords,
                            /*isPremultiplied=*/param.isPremultiplied);
                    _bindingMap[name] = texelBinding; // used for non-bindless

                    HdBinding layoutBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT,
                                    bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_PTEX_LAYOUT,
                                    locator.uniformLocation++,
                                    locator.textureUnit++);

                    const TfToken glLayoutName(_ConcatLayout(glName));
                    metaDataOut->shaderParameterBinding[layoutBinding] =
                        MetaData::ShaderParameterAccessor(
                            /*name=*/glLayoutName,
                            /*type=*/HdStGLConversions::GetGLSLTypename(
                                HdType::HdTypeInt32));

                    // Layout for Ptex
                    const TfToken layoutName(_ConcatLayout(name));
                    // used for non-bindless
                    _bindingMap[layoutName] = layoutBinding; 
                } else if (param.textureType == HdTextureType::Udim) {
                    // Texture Array for UDIM
                    HdBinding textureBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_UDIM_ARRAY,
                                bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_UDIM_ARRAY,
                                locator.uniformLocation++,
                                locator.textureUnit++);

                    metaDataOut->shaderParameterBinding[textureBinding] =
                        MetaData::ShaderParameterAccessor(
                            /*name=*/param.name,
                            /*type=*/glType,
                            /*swizzle=*/glSwizzle,
                            /*inPrimvars=*/param.samplerCoords,
                            /*isPremultiplied=*/param.isPremultiplied);
                    // used for non-bindless
                    _bindingMap[param.name] = textureBinding;

                    // Layout for UDIM
                    const TfToken layoutName(_ConcatLayout(param.name));

                    HdBinding layoutBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_UDIM_LAYOUT,
                            bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_UDIM_LAYOUT,
                            locator.uniformLocation++,
                            locator.textureUnit++);

                    metaDataOut->shaderParameterBinding[layoutBinding] =
                        MetaData::ShaderParameterAccessor(
                            /*name=*/layoutName,
                            /*type=*/HdStGLConversions::GetGLSLTypename(
                                HdType::HdTypeFloat));

                    // used for non-bindless
                    _bindingMap[layoutName] = layoutBinding;
                } else if (param.textureType == HdTextureType::Uv) {
                    if (param.IsArrayOfTextures()) {
                        size_t const numTextures = param.arrayOfTexturesSize;
                        
                        // Create binding for each texture in array of textures.
                        HdBinding firstBinding;
                        for (size_t i = 0; i < numTextures; i++) {
                            HdBinding textureBinding = bindless
                                ? HdBinding(
                                    HdBinding::BINDLESS_ARRAY_OF_TEXTURE_2D,
                                    bindlessTextureLocation++)
                                : HdBinding(
                                    HdBinding::ARRAY_OF_TEXTURE_2D,
                                    locator.uniformLocation++,
                                    locator.textureUnit++);
                            if (i == 0) {
                                firstBinding = textureBinding;
                            }            
                            TfToken const indexedName(
                                name.GetString() + std::to_string(i));
                            // used for non-bindless
                            _bindingMap[indexedName] = textureBinding;
                        } 

                        // Only fill metadata for the first binding.
                        metaDataOut->shaderParameterBinding[firstBinding] =
                            MetaData::ShaderParameterAccessor(
                                /*name=*/glName,
                                /*type=*/glType,
                                /*swizzle=*/glSwizzle,
                                /*inPrimvars=*/param.samplerCoords,
                                /*isPremultiplied=*/param.isPremultiplied,
                                /*processTextureFallbackValue=*/isMaterialShader,
                                /*arrayOfTexturesSize*/numTextures);
                    } else {
                        // 2d texture
                        HdBinding textureBinding = bindless
                            ? HdBinding(HdBinding::BINDLESS_TEXTURE_2D,
                                        bindlessTextureLocation++)
                            : HdBinding(HdBinding::TEXTURE_2D,
                                        locator.uniformLocation++,
                                        locator.textureUnit++);

                        metaDataOut->shaderParameterBinding[textureBinding] =
                            MetaData::ShaderParameterAccessor(
                                /*name=*/glName,
                                /*type=*/glType,
                                /*swizzle=*/glSwizzle,
                                /*inPrimvars=*/param.samplerCoords,
                                /*isPremultiplied=*/param.isPremultiplied,
                                /*processTextureFallbackValue=*/isMaterialShader);
                        // used for non-bindless
                        _bindingMap[name] = textureBinding;
                    }
                } else if (param.textureType == HdTextureType::Field) {
                    // 3d texture
                    HdBinding textureBinding = bindless
                        ? HdBinding(HdBinding::BINDLESS_TEXTURE_FIELD,
                                    bindlessTextureLocation++)
                        : HdBinding(HdBinding::TEXTURE_FIELD,
                                    locator.uniformLocation++,
                                    locator.textureUnit++);

                    metaDataOut->shaderParameterBinding[textureBinding] =
                        MetaData::ShaderParameterAccessor(
                            /*name=*/glName,
                            /*type=*/glType,
                            /*swizzle=*/glSwizzle,
                            /*inPrimvars=*/param.samplerCoords,
                            /*isPremultiplied=*/param.isPremultiplied,
                            /*processTextureFallbackValue=*/isMaterialShader);
                    _bindingMap[name] = textureBinding; // used for non-bindless
                }
            } else if (param.IsPrimvarRedirect() || param.IsFieldRedirect()) {
                TfTokenVector const& samplePrimvars = param.samplerCoords;
                TfTokenVector glNames;
                glNames.reserve(samplePrimvars.size());
                for (auto const& pv : samplePrimvars) {
                    glNames.push_back(HdStGLConversions::GetGLSLIdentifier(pv));
                }

                HdBinding binding = param.IsPrimvarRedirect()
                    ? HdBinding(HdBinding::PRIMVAR_REDIRECT,
                                shaderPrimvarRedirectLocation++)
                    : HdBinding(HdBinding::FIELD_REDIRECT,
                                shaderFieldRedirectLocation++);
                
                metaDataOut->shaderParameterBinding[binding]
                    = MetaData::ShaderParameterAccessor(
                    /*name=*/glName,
                    /*type=*/glType,
                    /*swizzle=*/glSwizzle,
                    /*inPrimvars=*/glNames);
            } else if (param.IsTransform2d()) {
                HdBinding binding = HdBinding(HdBinding::TRANSFORM_2D,
                                              shaderTransform2dLocation++);
                metaDataOut->shaderParameterBinding[binding] =
                    MetaData::ShaderParameterAccessor(
                        /*name=*/glName,
                        /*type=*/glType,
                        /*swizzle=*/glSwizzle,
                        /*inPrimvars=*/param.samplerCoords);            
            } else if (param.IsAdditionalPrimvar()) {
                // Additional primvars is used so certain primvars survive
                // primvar filtering. We can ignore them here, because primvars
                // found on the drawItem are already processed further above.
            } else {
                TF_CODING_ERROR("Can't resolve %s", param.name.GetText());
            }
        }
    }

    // Add custom bindings.
    // Don't need to sanitize the name used, since these are internally
    // generated.
    TF_FOR_ALL (it, customBindings) {
        if (it->IsInterleavedBufferArray()) {
            // Interleaved resource, only need a single binding point
            HdBinding binding = locator.GetBinding(it->GetBindingType(),
                                                   it->GetName());
            bool const concatenateNames = it->ConcatenateNames();

            MetaData::StructBlock sblock(it->GetName(), it->GetArraySize());

            HdBufferArrayRangeSharedPtr bar_ = it->GetBar();
            HdStBufferArrayRangeSharedPtr bar =
                std::static_pointer_cast<HdStBufferArrayRange> (bar_);

            for (auto const& nameRes : bar->GetResources()) {
                HdTupleType valueType = nameRes.second->GetTupleType();
                TfToken glType =
                    HdStGLConversions::GetGLSLTypename(valueType.type);
                sblock.entries.emplace_back(nameRes.first,
                                            glType,
                                             nameRes.second->GetOffset(),
                                            valueType.count,
                                            concatenateNames);
            }
            sblock.arraySize = it->GetArraySize();
            metaDataOut->customInterleavedBindings.insert(
                std::make_pair(binding, sblock));
            _bindingMap[it->GetName()] = binding;
        } else {
            // Non interleaved resource
            typedef MetaData::BindingDeclaration BindingDeclaration;
            if (it->IsBufferArray()) {
                // The BAR was provided, so we will record the name, dataType,
                // binding type and binding location.

                HdBufferArrayRangeSharedPtr bar_ = it->GetBar();
                HdStBufferArrayRangeSharedPtr bar =
                    std::static_pointer_cast<HdStBufferArrayRange> (bar_);

                for (auto const& nameRes : bar->GetResources()) {
                    HdBinding binding = locator.GetBinding(it->GetBindingType(), nameRes.first);
                    BindingDeclaration b(nameRes.first,
                        HdStGLConversions::GetGLSLTypename(
                            nameRes.second->GetTupleType().type),
                        binding,
                        it->isWritable());
                    metaDataOut->customBindings.push_back(b);
                    _bindingMap[nameRes.first] = binding;
                }
            } else {
                HdBinding binding = locator.GetBinding(it->GetBindingType(), it->GetName());
                BindingDeclaration b(it->GetName(),
                                     HdStGLConversions::GetGLSLTypename(
                                                    it->GetDataType()),
                                     binding,
                                     it->isWritable());

                // note that GetDataType() may return HdTypeInvalid,
                // in case it's a typeless binding. CodeGen generates
                // declarations and accessors only for BindingDeclaration
                // with a valid type.
                metaDataOut->customBindings.push_back(b);
                _bindingMap[it->GetName()] = binding;
            }
        }
    }
}

void
HdSt_ResourceBinder::ResolveComputeBindings(
                    HdBufferSpecVector const &readWriteBufferSpecs,
                    HdBufferSpecVector const &readOnlyBufferSpecs,
                    HdStShaderCodeSharedPtrVector const &shaders,
                    MetaData *metaDataOut,
                    HgiCapabilities const *capabilities)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(metaDataOut)) {
        return;
    }

    // GL context caps
    HdBinding::Type bindingType = 
        capabilities->IsSet(HgiDeviceCapabilitiesBitsBindlessBuffers) ?
            HdBinding::BINDLESS_SSBO_RANGE : HdBinding::SSBO;

    // binding assignments
    BindingLocator locator;

    // clear all
    _bindingMap.clear();
    
    // read-write per prim data
    for (HdBufferSpec const& spec: readWriteBufferSpecs) {
        HdBinding binding = locator.GetBinding(bindingType, spec.name);
        _bindingMap[spec.name] = binding;
        metaDataOut->computeReadWriteData[binding] =
            MetaData::Primvar(spec.name,
                              HdStGLConversions::GetGLSLTypename(
                                             spec.tupleType.type));
    }
    
    // read-only per prim data
    for (HdBufferSpec const& spec: readOnlyBufferSpecs) {
        HdBinding binding = locator.GetBinding(bindingType, spec.name);
        _bindingMap[spec.name] = binding;
        metaDataOut->computeReadOnlyData[binding] =
            MetaData::Primvar(spec.name,
                              HdStGLConversions::GetGLSLTypename(
                                             spec.tupleType.type));
    }
}

////////////////////////////////////////////////////////////
// Hgi Binding
////////////////////////////////////////////////////////////

void
HdSt_ResourceBinder::GetBufferBindingDesc(
    HgiResourceBindingsDesc * bindingsDesc,
    TfToken const & name,
    HdStBufferResourceSharedPtr const & resource,
    int offset,
    int level,
    int numElements) const
{
    if (!resource || !resource->GetHandle()) return;

    HdBinding binding = GetBinding(name, level);

    HgiShaderStage stageUsage =
        HgiShaderStageVertex | HgiShaderStageFragment |
        HgiShaderStagePostTessellationVertex;
    HgiBufferBindDesc desc;

    switch (binding.GetType()) {
    case HdBinding::SSBO:
        // Bind the entire buffer at offset zero.
        desc.buffers = { resource->GetHandle() };
        desc.offsets = { 0 };
        desc.sizes = { 0 };
        desc.bindingIndex = static_cast<uint32_t>(binding.GetLocation());
        desc.resourceType = HgiBindResourceTypeStorageBuffer;
        desc.stageUsage = stageUsage;
        bindingsDesc->buffers.push_back(desc);
        break;
    case HdBinding::UBO:
        // Bind numElements slices of the buffer at the specified offset.
        desc.buffers = { resource->GetHandle() };
        desc.offsets = { static_cast<uint32_t>(offset) };
        desc.sizes =
            { static_cast<uint32_t>(numElements * resource->GetStride()) };
        desc.bindingIndex = static_cast<uint32_t>(binding.GetLocation());
        desc.resourceType = HgiBindResourceTypeUniformBuffer;
        desc.stageUsage = stageUsage;
        bindingsDesc->buffers.push_back(desc);
        break;
    default:
        // Do nothing here for other binding types.
        break;
    }
}

void
HdSt_ResourceBinder::GetBufferArrayBindingDesc(
    HgiResourceBindingsDesc * bindingsDesc,
    HdStBufferArrayRangeSharedPtr const & bar) const
{
    if (!bar) return;

    for (auto const & it : bar->GetResources()) {
        GetBufferBindingDesc(bindingsDesc,
                             it.first,
                             it.second, it.second->GetOffset());
    }
}

void
HdSt_ResourceBinder::GetInterleavedBufferArrayBindingDesc(
    HgiResourceBindingsDesc *bindingsDesc,
    HdStBufferArrayRangeSharedPtr const & bar,
    TfToken const & name) const
{
    if (!bar) return;

    GetBufferBindingDesc(bindingsDesc,
                         name,
                         bar->GetResource(),
                         bar->GetResource()->GetOffset());
}

void
HdSt_ResourceBinder::GetInstanceBufferArrayBindingDesc(
    HgiResourceBindingsDesc * bindingsDesc,
    HdStBufferArrayRangeSharedPtr const & bar,
    int level) const
{
    if (!bar) return;

    for (auto const & it : bar->GetResources()) {
        if (HasBinding(it.first, level)) {
            GetBufferBindingDesc(
                bindingsDesc,
                it.first, it.second, it.second->GetOffset(), level);
        }
    }
}

void
HdSt_ResourceBinder::GetBindingRequestBindingDesc(
    HgiResourceBindingsDesc * bindingsDesc,
    HdBindingRequest const & req) const
{
    if (req.IsTypeless()) {
        return;
    } else if (req.IsResource()) {
        HdStBufferResourceSharedPtr resource =
            std::static_pointer_cast<HdStBufferResource>(req.GetResource());

        GetBufferBindingDesc(bindingsDesc,
                             req.GetName(),
                             resource,
                             req.GetByteOffset());

    } else if (req.IsInterleavedBufferArray()) {
        // note: interleaved buffer needs only 1 binding
        HdStBufferArrayRangeSharedPtr bar =
            std::static_pointer_cast<HdStBufferArrayRange>(req.GetBar());

        GetBufferBindingDesc(bindingsDesc,
                             req.GetName(),
                             bar->GetResource(),
                             req.GetByteOffset());

    } else if (req.IsBufferArray()) {
        HdStBufferArrayRangeSharedPtr bar =
            std::static_pointer_cast<HdStBufferArrayRange>(req.GetBar());

        GetBufferArrayBindingDesc(bindingsDesc, bar);
    }
}

void
HdSt_ResourceBinder::GetTextureBindingDesc(
    HgiResourceBindingsDesc * bindingsDesc,
    TfToken const & name,
    HgiSamplerHandle const & texelSampler,
    HgiTextureHandle const & texelTexture) const
{
    if (!texelSampler.Get() || !texelTexture.Get()) {
        return;
    }

    HdBinding const binding = GetBinding(name);

    HgiTextureBindDesc texelDesc;
    texelDesc.stageUsage =
        HgiShaderStageGeometry | HgiShaderStageFragment |
        HgiShaderStagePostTessellationVertex;
    texelDesc.textures = { texelTexture };
    texelDesc.samplers = { texelSampler };
    texelDesc.resourceType = HgiBindResourceTypeSampledImage;
    texelDesc.bindingIndex = binding.GetTextureUnit();
    bindingsDesc->textures.push_back(std::move(texelDesc));
}

void
HdSt_ResourceBinder::GetTextureWithLayoutBindingDesc(
    HgiResourceBindingsDesc * bindingsDesc,
    TfToken const & name,
    HgiSamplerHandle const & texelSampler,
    HgiTextureHandle const & texelTexture,
    HgiTextureHandle const & layoutTexture) const
{
    if (!texelSampler.Get() || !texelTexture.Get() || !layoutTexture.Get()) {
        return;
    }

    GetTextureBindingDesc(bindingsDesc, name, texelSampler, texelTexture);

    HdBinding const layoutBinding = GetBinding(_ConcatLayout(name));
    HgiTextureBindDesc layoutDesc;
    layoutDesc.textures = { layoutTexture };
    layoutDesc.samplers = { };
    layoutDesc.resourceType = HgiBindResourceTypeSampledImage;
    layoutDesc.bindingIndex = layoutBinding.GetTextureUnit();
    bindingsDesc->textures.push_back(std::move(layoutDesc));
}

////////////////////////////////////////////////////////////
// GL Binding
////////////////////////////////////////////////////////////

void
HdSt_ResourceBinder::BindBuffer(TfToken const &name,
                              HdStBufferResourceSharedPtr const &buffer) const
{
    BindBuffer(name, buffer, buffer->GetOffset(), /*level=*/-1);
}

void
HdSt_ResourceBinder::BindBuffer(TfToken const &name,
                              HdStBufferResourceSharedPtr const &buffer,
                              int offset,
                              int level,
                              int numElements) const
{
    HD_TRACE_FUNCTION();

    // it is possible that the buffer has not been initialized when
    // the instanceIndex is empty (e.g. FX points. see bug 120354)
    if (!buffer->GetHandle()) return;

    HdBinding binding = GetBinding(name, level);
    HdBinding::Type type = binding.GetType();
    int loc              = binding.GetLocation();

    HdTupleType tupleType = buffer->GetTupleType();

    void const* offsetPtr =
        reinterpret_cast<const void*>(
            static_cast<intptr_t>(offset));
    switch(type) {
    case HdBinding::VERTEX_ATTR:
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetHandle()->GetRawResource());
        glVertexAttribPointer(loc,
                  _GetNumComponents(tupleType.type),
                  HdStGLConversions::GetGLAttribType(tupleType.type),
                  _ShouldBeNormalized(tupleType.type),
                              buffer->GetStride(),
                              offsetPtr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(loc);
        break;
    case HdBinding::DRAW_INDEX:
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetHandle()->GetRawResource());
        glVertexAttribIPointer(loc,
                               HdGetComponentCount(tupleType.type),
                               GL_INT,
                               buffer->GetStride(),
                               offsetPtr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glEnableVertexAttribArray(loc);
        break;
    case HdBinding::DRAW_INDEX_INSTANCE:
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetHandle()->GetRawResource());
        glVertexAttribIPointer(loc,
                               HdGetComponentCount(tupleType.type),
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
        glBindBuffer(GL_ARRAY_BUFFER, buffer->GetHandle()->GetRawResource());
        // instancerNumLevels is represented by the tuple size.
        // We unroll this to an array of int[1] attributes.
        for (size_t i = 0; i < buffer->GetTupleType().count; ++i) {
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
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                     buffer->GetHandle()->GetRawResource());
        break;
    case HdBinding::BINDLESS_UNIFORM:
        // at least in nvidia driver 346.59, this query call doesn't show
        // any pipeline stall.
        if (!glIsNamedBufferResidentNV(buffer->GetHandle()->GetRawResource())) {
            glMakeNamedBufferResidentNV(
                buffer->GetHandle()->GetRawResource(), GL_READ_WRITE);
        }
        {
            HgiGLBuffer * bufferGL =
                static_cast<HgiGLBuffer*>(buffer->GetHandle().Get());
            glUniformui64NV(loc, bufferGL->GetBindlessGPUAddress());
        }
        break;
    case HdBinding::SSBO:
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, loc,
                         buffer->GetHandle()->GetRawResource());
        break;
    case HdBinding::BINDLESS_SSBO_RANGE:
        // at least in nvidia driver 346.59, this query call doesn't show
        // any pipeline stall.
        if (!glIsNamedBufferResidentNV(buffer->GetHandle()->GetRawResource())) {
            glMakeNamedBufferResidentNV(
                buffer->GetHandle()->GetRawResource(), GL_READ_WRITE);
        }
        {
            HgiGLBuffer * bufferGL =
                static_cast<HgiGLBuffer*>(buffer->GetHandle().Get());
            glUniformui64NV(loc, bufferGL->GetBindlessGPUAddress()+offset);
        }
        break;
    case HdBinding::DISPATCH:
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER,
                     buffer->GetHandle()->GetRawResource());
        break;
    case HdBinding::UBO:
    case HdBinding::UNIFORM:
        glBindBufferRange(GL_UNIFORM_BUFFER, loc,
                          buffer->GetHandle()->GetRawResource(),
                          offset,
                          buffer->GetStride() * numElements);
        break;
    case HdBinding::TEXTURE_2D:
    case HdBinding::TEXTURE_FIELD:
        // nothing
        break;
    default:
        TF_CODING_ERROR("binding type %d not found for %s",
                        type, name.GetText());
        break;
    }
}

void
HdSt_ResourceBinder::UnbindBuffer(TfToken const &name,
                                HdStBufferResourceSharedPtr const &buffer,
                                int level) const
{
    HD_TRACE_FUNCTION();

    // it is possible that the buffer has not been initialized when
    // the instanceIndex is empty (e.g. FX points)
    if (!buffer->GetHandle()) return;

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
        // instancerNumLevels is represented by the tuple size.
        for (size_t i = 0; i < buffer->GetTupleType().count; ++i) {
            glDisableVertexAttribArray(loc);
            glVertexAttribDivisor(loc, 0);
            ++loc;
        }
        break;
    case HdBinding::INDEX_ATTR:
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        break;
    case HdBinding::BINDLESS_UNIFORM:
        if (glIsNamedBufferResidentNV(buffer->GetHandle()->GetRawResource())) {
            glMakeNamedBufferNonResidentNV(
                buffer->GetHandle()->GetRawResource());
        }
        break;
    case HdBinding::SSBO:
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, loc, 0);
        break;
    case HdBinding::BINDLESS_SSBO_RANGE:
        if (glIsNamedBufferResidentNV(buffer->GetHandle()->GetRawResource())) {
            glMakeNamedBufferNonResidentNV(
                buffer->GetHandle()->GetRawResource());
        }
        break;
    case HdBinding::DISPATCH:
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        break;
    case HdBinding::UBO:
    case HdBinding::UNIFORM:
        glBindBufferBase(GL_UNIFORM_BUFFER, loc, 0);
        break;
    case HdBinding::TEXTURE_2D:
    case HdBinding::TEXTURE_FIELD:
        // nothing
        break;
    default:
        TF_CODING_ERROR("binding type %d not found for %s",
                        type, name.GetText());
        break;
    }
}

void
HdSt_ResourceBinder::BindConstantBuffer(
    HdStBufferArrayRangeSharedPtr const &constantBar) const
{
    if (!constantBar) return;

    // constant buffer is interleaved. we just need to bind a buffer.
    BindBuffer(_tokens->constantPrimvars, constantBar->GetResource());
}

void
HdSt_ResourceBinder::UnbindConstantBuffer(
    HdStBufferArrayRangeSharedPtr const &constantBar) const
{
    if (!constantBar) return;

    UnbindBuffer(_tokens->constantPrimvars, constantBar->GetResource());
}

void
HdSt_ResourceBinder::BindInterleavedBuffer(
    HdStBufferArrayRangeSharedPtr const &interleavedBar,
    TfToken const &name) const
{
    if (!interleavedBar) return;

    BindBuffer(name, interleavedBar->GetResource());
}

void
HdSt_ResourceBinder::UnbindInterleavedBuffer(
    HdStBufferArrayRangeSharedPtr const &interleavedBar,
    TfToken const &name) const
{
    if (!interleavedBar) return;

    UnbindBuffer(name, interleavedBar->GetResource());
}

void
HdSt_ResourceBinder::BindInstanceBufferArray(
    HdStBufferArrayRangeSharedPtr const &bar, int level) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        if (HasBinding(it->first, level)) {
            BindBuffer(it->first, it->second, it->second->GetOffset(), level);
        }
    }
}

void
HdSt_ResourceBinder::UnbindInstanceBufferArray(
    HdStBufferArrayRangeSharedPtr const &bar, int level) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        if (HasBinding(it->first, level)) {
            UnbindBuffer(it->first, it->second, level);
        }
    }
}

void
HdSt_ResourceBinder::BindShaderResources(HdStShaderCode const *shader) const
{
}

void
HdSt_ResourceBinder::UnbindShaderResources(HdStShaderCode const *shader) const
{
}

void
HdSt_ResourceBinder::BindBufferArray(HdStBufferArrayRangeSharedPtr const &bar) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        BindBuffer(it->first, it->second);
    }
}

void
HdSt_ResourceBinder::Bind(HdBindingRequest const& req) const
{
    if (req.IsTypeless()) {
        return;
    } else if (req.IsResource()) {
        HdBufferResourceSharedPtr res_ = req.GetResource();
        HdStBufferResourceSharedPtr res =
            std::static_pointer_cast<HdStBufferResource> (res_);

        BindBuffer(req.GetName(), res, req.GetByteOffset());
    } else if (req.IsInterleavedBufferArray()) {
        // note: interleaved buffer needs only 1 binding
        HdBufferArrayRangeSharedPtr bar_ = req.GetBar();
        HdStBufferArrayRangeSharedPtr bar =
            std::static_pointer_cast<HdStBufferArrayRange> (bar_);
        BindBuffer(req.GetName(), bar->GetResource(), req.GetByteOffset(), 
            /*level*/-1, bar->GetNumElements());
    } else if (req.IsBufferArray()) {
        HdBufferArrayRangeSharedPtr bar_ = req.GetBar();
        HdStBufferArrayRangeSharedPtr bar =
            std::static_pointer_cast<HdStBufferArrayRange> (bar_);
        BindBufferArray(bar);
    }
}

void
HdSt_ResourceBinder::Unbind(HdBindingRequest const& req) const
{
    if (req.IsTypeless()) {
        return;
    } else if (req.IsResource()) {
        HdBufferResourceSharedPtr res_ = req.GetResource();
        HdStBufferResourceSharedPtr res =
            std::static_pointer_cast<HdStBufferResource> (res_);

        UnbindBuffer(req.GetName(), res);
    } else if (req.IsInterleavedBufferArray()) {
        // note: interleaved buffer needs only 1 binding
        HdBufferArrayRangeSharedPtr bar_ = req.GetBar();
        HdStBufferArrayRangeSharedPtr bar =
            std::static_pointer_cast<HdStBufferArrayRange> (bar_);

        UnbindBuffer(req.GetName(), bar->GetResource());
    } else if (req.IsBufferArray()) {
        HdBufferArrayRangeSharedPtr bar_ = req.GetBar();
        HdStBufferArrayRangeSharedPtr bar =
            std::static_pointer_cast<HdStBufferArrayRange> (bar_);

        UnbindBufferArray(bar);
    }
}

void
HdSt_ResourceBinder::UnbindBufferArray(
    HdStBufferArrayRangeSharedPtr const &bar) const
{
    if (!bar) return;

    TF_FOR_ALL(it, bar->GetResources()) {
        UnbindBuffer(it->first, it->second);
    }
}

void
HdSt_ResourceBinder::BindUniformi(TfToken const &name,
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
HdSt_ResourceBinder::BindUniformArrayi(TfToken const &name,
                                 int count, const int *value) const
{
    HdBinding uniformLocation = GetBinding(name);
    if (uniformLocation.GetLocation() == HdBinding::NOT_EXIST) return;

    TF_VERIFY(uniformLocation.IsValid());
    TF_VERIFY(uniformLocation.GetType() == HdBinding::UNIFORM_ARRAY);

    glUniform1iv(uniformLocation.GetLocation(), count, value);
}

void
HdSt_ResourceBinder::BindUniformui(TfToken const &name,
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
HdSt_ResourceBinder::BindUniformf(TfToken const &name,
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

HdSt_ResourceBinder::MetaData::ID
HdSt_ResourceBinder::MetaData::ComputeHash() const
{
    ID hash = 0;
    
    boost::hash_combine(hash, drawingCoord0Binding.binding.GetValue());
    boost::hash_combine(hash, drawingCoord0Binding.dataType);
    boost::hash_combine(hash, drawingCoord1Binding.binding.GetValue());
    boost::hash_combine(hash, drawingCoord1Binding.dataType);
    boost::hash_combine(hash, drawingCoord2Binding.binding.GetValue());
    boost::hash_combine(hash, drawingCoord2Binding.dataType);
    boost::hash_combine(hash, drawingCoordIBinding.binding.GetValue());
    boost::hash_combine(hash, drawingCoordIBinding.dataType);
    boost::hash_combine(hash, instanceIndexArrayBinding.binding.GetValue());
    boost::hash_combine(hash, instanceIndexArrayBinding.dataType);
    boost::hash_combine(hash, instanceIndexBaseBinding.binding.GetValue());
    boost::hash_combine(hash, instanceIndexBaseBinding.dataType);
    boost::hash_combine(hash, primitiveParamBinding.binding.GetValue());
    boost::hash_combine(hash, primitiveParamBinding.dataType);
    boost::hash_combine(hash, edgeIndexBinding.binding.GetValue());
    boost::hash_combine(hash, edgeIndexBinding.dataType);
    boost::hash_combine(hash, coarseFaceIndexBinding.binding.GetValue());
    boost::hash_combine(hash, coarseFaceIndexBinding.dataType);

    TF_FOR_ALL(binDecl, fvarIndicesBindings) {
        boost::hash_combine(hash, binDecl->binding.GetValue());
        boost::hash_combine(hash, binDecl->dataType);
    }
    TF_FOR_ALL(binDecl, fvarPatchParamBindings) {
        boost::hash_combine(hash, binDecl->binding.GetValue());
        boost::hash_combine(hash, binDecl->dataType);
    }

    // separators are inserted to distinguish primvars have a same layout
    // but different interpolation.
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL(binDecl, customBindings) {
        boost::hash_combine(hash, binDecl->name.Hash());
        boost::hash_combine(hash, binDecl->dataType);
        boost::hash_combine(hash, binDecl->binding.GetType());
        boost::hash_combine(hash, binDecl->binding.GetLocation());
    }

    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL(blockIt, customInterleavedBindings) {
        boost::hash_combine(hash, (int)blockIt->first.GetType()); // binding
        TF_FOR_ALL (it, blockIt->second.entries) {
            StructEntry const &entry = *it;
            boost::hash_combine(hash, entry.name.Hash());
            boost::hash_combine(hash, entry.dataType);
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
            boost::hash_combine(hash, entry.dataType);
            boost::hash_combine(hash, entry.offset);
            boost::hash_combine(hash, entry.arraySize);
        }
    }

    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (blockIt, topologyVisibilityData) {
        boost::hash_combine(hash, (int)blockIt->first.GetType()); // binding
        TF_FOR_ALL (it, blockIt->second.entries) {
            StructEntry const &entry = *it;
            boost::hash_combine(hash, entry.name.Hash());
            boost::hash_combine(hash, entry.dataType);
            boost::hash_combine(hash, entry.offset);
            boost::hash_combine(hash, entry.arraySize);
        }
    }

    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, instanceData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        NestedPrimvar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType);
        boost::hash_combine(hash, primvar.level);
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, vertexData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        Primvar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType);
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, varyingData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        Primvar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType);
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, elementData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        Primvar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType);
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, fvarData) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        FvarPrimvar const &primvar = it->second;
        boost::hash_combine(hash, primvar.name.Hash());
        boost::hash_combine(hash, primvar.dataType);
        boost::hash_combine(hash, primvar.channel);
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (blockIt, shaderData) {
        boost::hash_combine(hash, (int)blockIt->first.GetType()); // binding
        TF_FOR_ALL (it, blockIt->second.entries) {
            StructEntry const &entry = *it;
            boost::hash_combine(hash, entry.name.Hash());
            boost::hash_combine(hash, entry.dataType);
            boost::hash_combine(hash, entry.offset);
            boost::hash_combine(hash, entry.arraySize);
        }
    }
    boost::hash_combine(hash, 0); // separator
    TF_FOR_ALL (it, shaderParameterBinding) {
        boost::hash_combine(hash, (int)it->first.GetType()); // binding
        ShaderParameterAccessor const &entry = it->second;
        boost::hash_combine(hash, entry.name.Hash());
        boost::hash_combine(hash, entry.dataType);
        boost::hash_combine(hash, entry.swizzle);
    }

    return hash;
}

/* static */
uint64_t
HdSt_ResourceBinder::GetSamplerBindlessHandle(
        HgiSamplerHandle const &samplerHandle,
        HgiTextureHandle const &textureHandle)
{
    HgiGLSampler * const glSampler =
        const_cast<HgiGLSampler*>(
        dynamic_cast<const HgiGLSampler*>(samplerHandle.Get()));

    HgiGLTexture * const glTexture =
        const_cast<HgiGLTexture*>(
        dynamic_cast<const HgiGLTexture*>(textureHandle.Get()));

    if (!glSampler || !glTexture) {
        return 0;
    }

    return glSampler->GetBindlessHandle(textureHandle);
}

/* static */
uint64_t
HdSt_ResourceBinder::GetTextureBindlessHandle(
        HgiTextureHandle const &textureHandle)
{
    HgiGLTexture * const glTexture =
        const_cast<HgiGLTexture*>(
        dynamic_cast<const HgiGLTexture*>(textureHandle.Get()));

    if (!glTexture) {
        return 0;
    }

    return glTexture->GetBindlessHandle();
}

static
bool
_IsBindless(HdBinding const & binding)
{
    switch (binding.GetType()) {
        case HdBinding::BINDLESS_TEXTURE_2D:
        case HdBinding::BINDLESS_ARRAY_OF_TEXTURE_2D:
        case HdBinding::BINDLESS_TEXTURE_FIELD:
        case HdBinding::BINDLESS_TEXTURE_UDIM_ARRAY:
        case HdBinding::BINDLESS_TEXTURE_UDIM_LAYOUT:
        case HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL:
        case HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT:
            return true;
        default:
            return false;;
    }
}

static
GLenum
_GetTextureTarget(HdBinding const & binding)
{
    switch (binding.GetType()) {
        case HdBinding::TEXTURE_2D:
        case HdBinding::ARRAY_OF_TEXTURE_2D:
            return GL_TEXTURE_2D;
        case HdBinding::TEXTURE_FIELD:
            return GL_TEXTURE_3D;
        case HdBinding::TEXTURE_UDIM_ARRAY:
        case HdBinding::TEXTURE_PTEX_TEXEL:
            return GL_TEXTURE_2D_ARRAY;
        case HdBinding::TEXTURE_UDIM_LAYOUT:
            return GL_TEXTURE_1D;
        case HdBinding::TEXTURE_PTEX_LAYOUT:
            return GL_TEXTURE_1D_ARRAY;
        default:
            TF_CODING_ERROR("Unknown texture binding type");
            return GL_NONE;
    }
}

void
HdSt_ResourceBinder::BindTexture(
        const TfToken &name,
        HgiSamplerHandle const &samplerHandle,
        HgiTextureHandle const &textureHandle,
        const bool bind) const
{
    const HdBinding binding = GetBinding(name);
    if (_IsBindless(binding)) {
        return;
    }

    const int samplerUnit = binding.GetTextureUnit();

    glActiveTexture(GL_TEXTURE0 + samplerUnit);

    const HgiTexture * const tex = textureHandle.Get();
    const HgiGLTexture * const glTex =
        dynamic_cast<const HgiGLTexture*>(tex);

    if (tex && !glTex) {
        TF_CODING_ERROR("Resource binder only supports OpenGL");
    }

    const GLuint texName =
        (bind && glTex) ? glTex->GetTextureId() : 0;
    glBindTexture(_GetTextureTarget(binding), texName);

    const HgiSampler * const sampler = samplerHandle.Get();
    const HgiGLSampler * const glSampler =
        dynamic_cast<const HgiGLSampler*>(sampler);

    if (sampler && !glSampler) {
        TF_CODING_ERROR("Resource binder only supports OpenGL");
    }

    const GLuint samplerName =
        (bind && glSampler) ? glSampler->GetSamplerId() : 0;
    glBindSampler(samplerUnit, samplerName);

    glActiveTexture(GL_TEXTURE0);
}

void
HdSt_ResourceBinder::BindTextureWithLayout(
        TfToken const &name,
        HgiSamplerHandle const &texelSampler,
        HgiTextureHandle const &texelTexture,
        HgiTextureHandle const &layoutTexture,
        const bool bind) const
{
    const HdBinding texelBinding = GetBinding(name);
    if (_IsBindless(texelBinding)) {
        return;
    }

    const int texelSamplerUnit = texelBinding.GetTextureUnit();

    glActiveTexture(GL_TEXTURE0 + texelSamplerUnit);
    glBindTexture(_GetTextureTarget(texelBinding),
              (bind && texelTexture) ? texelTexture->GetRawResource() : 0);

    const HgiGLSampler * const glSampler =
        bind ? dynamic_cast<HgiGLSampler*>(texelSampler.Get()) : nullptr;

    if (glSampler) {
        glBindSampler(texelSamplerUnit, (GLuint)glSampler->GetSamplerId());
    } else {
        glBindSampler(texelSamplerUnit, 0);
    }

    const HdBinding layoutBinding = GetBinding(_ConcatLayout(name));
    const int layoutSamplerUnit = layoutBinding.GetTextureUnit();

    glActiveTexture(GL_TEXTURE0 + layoutSamplerUnit);
    glBindTexture(_GetTextureTarget(layoutBinding),
              (bind && layoutTexture) ? layoutTexture->GetRawResource() : 0);
    glActiveTexture(GL_TEXTURE0);
}


PXR_NAMESPACE_CLOSE_SCOPE
