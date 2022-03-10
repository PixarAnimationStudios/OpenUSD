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
#include "pxr/imaging/glf/simpleShadowArray.h"

#include "pxr/imaging/hdSt/codeGen.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

#include <sstream>

#include <opensubdiv/osd/glslPatchShaderSource.h>

PXR_NAMESPACE_OPEN_SCOPE


namespace pxrImagingHdStCodeGen {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_double, "double"))
    ((_float, "float"))
    ((_int, "int"))
    ((_uint, "uint"))
    (hd_vec3)
    (hd_vec3_get)
    (hd_vec3_set)
    (hd_ivec3)
    (hd_ivec3_get)
    (hd_ivec3_set)
    (hd_dvec3)
    (hd_dvec3_get)
    (hd_dvec3_set)
    (hd_mat3)
    (hd_mat3_get)
    (hd_mat3_set)
    (hd_dmat3)
    (hd_dmat3_get)
    (hd_dmat3_set)
    (hd_vec4_2_10_10_10_get)
    (hd_vec4_2_10_10_10_set)
    (hd_half2_get)
    (hd_half2_set)
    (hd_half4_get)
    (hd_half4_set)
    (inPrimvars)
    (uvec2)
    (uvec3)
    (uvec4)
    (ivec2)
    (ivec3)
    (ivec4)
    (outPrimvars)
    (vec2)
    (vec3)
    (vec4)
    (dvec2)
    (dvec3)
    (dvec4)
    (mat3)
    (mat4)
    (dmat3)
    (dmat4)
    (packed_2_10_10_10)
    (packed_half2)
    (packed_half4)
    ((ptexTextureSampler, "ptexTextureSampler"))
    (isamplerBuffer)
    (samplerBuffer)
);

} // pxrImagingHdStCodeGen

HdSt_CodeGen::HdSt_CodeGen(HdSt_GeometricShaderPtr const &geometricShader,
                       HdStShaderCodeSharedPtrVector const &shaders,
                       TfToken const &materialTag)
    : _geometricShader(geometricShader), _shaders(shaders), 
      _materialTag(materialTag)
{
    TF_VERIFY(geometricShader);
}

HdSt_CodeGen::HdSt_CodeGen(HdStShaderCodeSharedPtrVector const &shaders)
    : _geometricShader(), _shaders(shaders)
{
}

HdSt_CodeGen::ID
HdSt_CodeGen::ComputeHash() const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    ID hash = _geometricShader ? _geometricShader->ComputeHash() : 0;
    boost::hash_combine(hash, _metaData.ComputeHash());
    boost::hash_combine(hash, HdStShaderCode::ComputeHash(_shaders));
    boost::hash_combine(hash, _materialTag.Hash());

    return hash;
}

static
std::string
_GetPtexTextureShaderSource()
{
    static std::string source =
        HioGlslfx(HdStPackagePtexTextureShader()).GetSource(
            pxrImagingHdStCodeGen::_tokens->ptexTextureSampler);
    return source;
}

// TODO: Shuffle code to remove these declarations.
static void _EmitDeclaration(std::stringstream &str,
                             TfToken const &name,
                             TfToken const &type,
                             HdBinding const &binding,
                             int arraySize=0);

static void _EmitStructAccessor(std::stringstream &str,
                                TfToken const &structName,
                                TfToken const &name,
                                TfToken const &type,
                                int arraySize,
                                const char *index = NULL,
                                bool concatenateNames = false);

static void _EmitComputeAccessor(std::stringstream &str,
                                 TfToken const &name,
                                 TfToken const &type,
                                 HdBinding const &binding,
                                 const char *index);

static void _EmitComputeMutator(std::stringstream &str,
                                TfToken const &name,
                                TfToken const &type,
                                HdBinding const &binding,
                                const char *index);

static void _EmitAccessor(std::stringstream &str,
                          TfToken const &name,
                          TfToken const &type,
                          HdBinding const &binding,
                          const char *index=NULL);
/*
  1. If the member is a scalar consuming N basic machine units,
  the base alignment is N.
  2. If the member is a two- or four-component vector with components
  consuming N basic machine units, the base alignment is 2N or 4N,
  respectively.
  3. If the member is a three-component vector with components
  consuming N basic machine units, the base alignment is 4N.
  4. If the member is an array of scalars or vectors, the base
  alignment and array stride are set to match the base alignment of
  a single array element, according to rules (1), (2), and (3), and
  rounded up to the base alignment of a vec4. The array may have
  padding at the end; the base offset of the member following the
  array is rounded up to the next multiple of the base alignment.

  9. If the member is a structure, the base alignment of the structure
  is <N>, where <N> is the largest base alignment value of any of its
  members, and rounded up to the base alignment of a vec4. The
  individual members of this sub-structure are then assigned offsets
  by applying this set of rules recursively, where the base offset of
  the first member of the sub-structure is equal to the aligned offset
  of the structure. The structure may have padding at the end; the
  base offset of the member following the sub-structure is rounded up
  to the next multiple of the base alignment of the structure.

  When using the std430 storage layout, shader storage blocks will be
  laid out in buffer storage identically to uniform and shader storage
  blocks using the std140 layout, except that the base alignment and
  stride of arrays of scalars and vectors in rule 4 and of structures
  in rule 9 are not rounded up a multiple of the base alignment of a
  vec4.

  i.e. rule 3 is still applied in std430. we use an array of 3-element
  struct instead of vec3/dvec3 to avoid this undesirable padding.

  struct instanceData0 {
    float x, y, z;
  }
  buffer buffer0 {
    instanceData0 data[];
  };
*/

static const char *
_GetPackedTypeDefinitions()
{
    return "struct hd_ivec3 { int    x, y, z; };\n"
           "struct hd_vec3  { float  x, y, z; };\n"
           "struct hd_dvec3 { double x, y, z; };\n"
           "struct hd_mat3  { float  m00, m01, m02,\n"
           "                         m10, m11, m12,\n"
           "                         m20, m21, m22; };\n"
           "struct hd_dmat3 { double m00, m01, m02,\n"
           "                         m10, m11, m12,\n"
           "                         m20, m21, m22; };\n"
           "ivec3 hd_ivec3_get(hd_ivec3 v) { return ivec3(v.x, v.y, v.z); }\n"
           "ivec3 hd_ivec3_get(ivec3 v)    { return v; }\n"
           "vec3  hd_vec3_get(hd_vec3 v)   { return vec3(v.x, v.y, v.z); }\n"
           "vec3  hd_vec3_get(vec3 v)      { return v; }\n"
           "dvec3 hd_dvec3_get(hd_dvec3 v) { return dvec3(v.x, v.y, v.z); }\n"
           "dvec3 hd_dvec3_get(dvec3 v)    { return v; }\n"
           "mat3  hd_mat3_get(hd_mat3 v)   { return mat3(v.m00, v.m01, v.m02,\n"
           "                                             v.m10, v.m11, v.m12,\n"
           "                                             v.m20, v.m21, v.m22); }\n"
           "mat3  hd_mat3_get(mat3 v)      { return v; }\n"
           "dmat3 hd_dmat3_get(hd_dmat3 v) { return dmat3(v.m00, v.m01, v.m02,\n"
           "                                              v.m10, v.m11, v.m12,\n"
           "                                              v.m20, v.m21, v.m22); }\n"
           "dmat3 hd_dmat3_get(dmat3 v)    { return v; }\n"
           "hd_ivec3 hd_ivec3_set(hd_ivec3 v) { return v; }\n"
           "hd_ivec3 hd_ivec3_set(ivec3 v)    { return hd_ivec3(v.x, v.y, v.z); }\n"
           "hd_vec3 hd_vec3_set(hd_vec3 v)    { return v; }\n"
           "hd_vec3 hd_vec3_set(vec3 v)       { return hd_vec3(v.x, v.y, v.z); }\n"
           "hd_dvec3 hd_dvec3_set(hd_dvec3 v) { return v; }\n"
           "hd_dvec3 hd_dvec3_set(dvec3 v)    { return hd_dvec3(v.x, v.y, v.z); }\n"
           "hd_mat3  hd_mat3_set(hd_mat3 v)   { return v; }\n"
           "hd_mat3  hd_mat3_set(mat3 v)      { return hd_mat3(v[0][0], v[0][1], v[0][2],\n"
           "                                                   v[1][0], v[1][1], v[1][2],\n"
           "                                                   v[2][0], v[2][1], v[2][2]); }\n"
           "hd_dmat3 hd_dmat3_set(hd_dmat3 v) { return v; }\n"
           "hd_dmat3 hd_dmat3_set(dmat3 v)    { return hd_dmat3(v[0][0], v[0][1], v[0][2],\n"
           "                                                    v[1][0], v[1][1], v[1][2],\n"
           "                                                    v[2][0], v[2][1], v[2][2]); }\n"
        // helper functions for 410 specification
        // applying a swizzle operator on int and float is not allowed in 410.
           "int hd_int_get(int v)          { return v; }\n"
           "int hd_int_get(ivec2 v)        { return v.x; }\n"
           "int hd_int_get(ivec3 v)        { return v.x; }\n"
           "int hd_int_get(ivec4 v)        { return v.x; }\n"
        // udim helper function
            "vec3 hd_sample_udim(vec2 v) {\n"
            "vec2 vf = floor(v);\n"
            "return vec3(v.x - vf.x, v.y - vf.y, clamp(vf.x, 0.0, 10.0) + 10.0 * vf.y);\n"
            "}\n"

        // -------------------------------------------------------------------
        // Packed HdType implementation.

        // XXX: this could be improved!
           "vec4 hd_vec4_2_10_10_10_get(int v) {\n"
           "    ivec4 unpacked = ivec4((v & 0x3ff) << 22, (v & 0xffc00) << 12,\n"
           "                           (v & 0x3ff00000) << 2, (v & 0xc0000000));\n"
           "    return vec4(unpacked) / 2147483647.0; }\n"
           "int hd_vec4_2_10_10_10_set(vec4 v) {\n"
           "    return ( (int(v.x * 511.0) & 0x3ff) |\n"
           "            ((int(v.y * 511.0) & 0x3ff) << 10) |\n"
           "            ((int(v.z * 511.0) & 0x3ff) << 20) |\n"
           "            ((int(v.w) & 0x1) << 30)); }\n"
        // half2 and half4 accessors (note that half and half3 are unsupported)
           "vec2 hd_half2_get(uint v) {\n"
           "    return unpackHalf2x16(v); }\n"
           "uint hd_half2_set(vec2 v) {\n"
           "    return packHalf2x16(v); }\n"
           "vec4 hd_half4_get(uvec2 v) {\n"
           "    return vec4(unpackHalf2x16(v.x), unpackHalf2x16(v.y)); }\n"
           "uvec2 hd_half4_set(vec4 v) {\n"
           "    return uvec2(packHalf2x16(v.xy), packHalf2x16(v.zw)); }\n"
           ;
}

static TfToken const &
_GetPackedType(TfToken const &token, bool packedAlignment)
{
    if (packedAlignment) {
        if (token == pxrImagingHdStCodeGen::_tokens->ivec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_ivec3;
        } else if (token == pxrImagingHdStCodeGen::_tokens->vec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_vec3;
        } else if (token == pxrImagingHdStCodeGen::_tokens->dvec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_dvec3;
        } else if (token == pxrImagingHdStCodeGen::_tokens->mat3) {
            return pxrImagingHdStCodeGen::_tokens->hd_mat3;
        } else if (token == pxrImagingHdStCodeGen::_tokens->dmat3) {
            return pxrImagingHdStCodeGen::_tokens->hd_dmat3;
        }
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_2_10_10_10) {
        return pxrImagingHdStCodeGen::_tokens->_int;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half2) {
        return pxrImagingHdStCodeGen::_tokens->_uint;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half4) {
        return pxrImagingHdStCodeGen::_tokens->uvec2;
    }
    return token;
}

static TfToken const &
_GetUnpackedType(TfToken const &token, bool packedAlignment)
{
    if (token == pxrImagingHdStCodeGen::_tokens->packed_2_10_10_10) {
        return pxrImagingHdStCodeGen::_tokens->vec4;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half2) {
        return pxrImagingHdStCodeGen::_tokens->vec2;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half4) {
        return pxrImagingHdStCodeGen::_tokens->vec4;
    }
    return token;
}

static TfToken const &
_GetPackedTypeAccessor(TfToken const &token, bool packedAlignment)
{
    if (packedAlignment) {
        if (token == pxrImagingHdStCodeGen::_tokens->ivec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_ivec3_get;
        } else if (token == pxrImagingHdStCodeGen::_tokens->vec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_vec3_get;
        } else if (token == pxrImagingHdStCodeGen::_tokens->dvec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_dvec3_get;
        } else if (token == pxrImagingHdStCodeGen::_tokens->mat3) {
            return pxrImagingHdStCodeGen::_tokens->hd_mat3_get;
        } else if (token == pxrImagingHdStCodeGen::_tokens->dmat3) {
            return pxrImagingHdStCodeGen::_tokens->hd_dmat3_get;
        }
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_2_10_10_10) {
        return pxrImagingHdStCodeGen::_tokens->hd_vec4_2_10_10_10_get;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half2) {
        return pxrImagingHdStCodeGen::_tokens->hd_half2_get;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half4) {
        return pxrImagingHdStCodeGen::_tokens->hd_half4_get;
    }
    return token;
}

static TfToken const &
_GetPackedTypeMutator(TfToken const &token, bool packedAlignment)
{
    if (packedAlignment) {
        if (token == pxrImagingHdStCodeGen::_tokens->ivec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_ivec3_set;
        } else if (token == pxrImagingHdStCodeGen::_tokens->vec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_vec3_set;
        } else if (token == pxrImagingHdStCodeGen::_tokens->dvec3) {
            return pxrImagingHdStCodeGen::_tokens->hd_dvec3_set;
        } else if (token == pxrImagingHdStCodeGen::_tokens->mat3) {
            return pxrImagingHdStCodeGen::_tokens->hd_mat3_set;
        } else if (token == pxrImagingHdStCodeGen::_tokens->dmat3) {
            return pxrImagingHdStCodeGen::_tokens->hd_dmat3_set;
        }
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_2_10_10_10) {
        return pxrImagingHdStCodeGen::_tokens->hd_vec4_2_10_10_10_set;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half2) {
        return pxrImagingHdStCodeGen::_tokens->hd_half2_set;
    }
    if (token == pxrImagingHdStCodeGen::_tokens->packed_half4) {
        return pxrImagingHdStCodeGen::_tokens->hd_half4_set;
    }
    return token;
}

static TfToken const &
_GetFlatType(TfToken const &token)
{
    if (token == pxrImagingHdStCodeGen::_tokens->ivec2) {
        return pxrImagingHdStCodeGen::_tokens->_int;
    } else if (token == pxrImagingHdStCodeGen::_tokens->ivec3) {
        return pxrImagingHdStCodeGen::_tokens->_int;
    } else if (token == pxrImagingHdStCodeGen::_tokens->ivec4) {
        return pxrImagingHdStCodeGen::_tokens->_int;
    } else if (token == pxrImagingHdStCodeGen::_tokens->vec2) {
        return pxrImagingHdStCodeGen::_tokens->_float;
    } else if (token == pxrImagingHdStCodeGen::_tokens->vec3) {
        return pxrImagingHdStCodeGen::_tokens->_float;
    } else if (token == pxrImagingHdStCodeGen::_tokens->vec4) {
        return pxrImagingHdStCodeGen::_tokens->_float;
    } else if (token == pxrImagingHdStCodeGen::_tokens->dvec2) {
        return pxrImagingHdStCodeGen::_tokens->_double;
    } else if (token == pxrImagingHdStCodeGen::_tokens->dvec3) {
        return pxrImagingHdStCodeGen::_tokens->_double;
    } else if (token == pxrImagingHdStCodeGen::_tokens->dvec4) {
        return pxrImagingHdStCodeGen::_tokens->_double;
    } else if (token == pxrImagingHdStCodeGen::_tokens->mat3) {
        return pxrImagingHdStCodeGen::_tokens->_float;
    } else if (token == pxrImagingHdStCodeGen::_tokens->mat4) {
        return pxrImagingHdStCodeGen::_tokens->_float;
    } else if (token == pxrImagingHdStCodeGen::_tokens->dmat3) {
        return pxrImagingHdStCodeGen::_tokens->_double;
    } else if (token == pxrImagingHdStCodeGen::_tokens->dmat4) {
        return pxrImagingHdStCodeGen::_tokens->_double;
    }
    return token;
}

namespace pxrImagingHdStCodeGen {
    struct LayoutQualifier {
        LayoutQualifier(HdBinding const &binding) :
            binding(binding) {
        }
        friend std::ostream & operator << (std::ostream & out,
                                           const LayoutQualifier &lq);
        HdBinding binding;
    };
    std::ostream & operator << (std::ostream & out, const LayoutQualifier &lq)
    {
        int location = lq.binding.GetLocation();

        switch (lq.binding.GetType()) {
        case HdBinding::VERTEX_ATTR:
        case HdBinding::DRAW_INDEX:
        case HdBinding::DRAW_INDEX_INSTANCE:
        case HdBinding::DRAW_INDEX_INSTANCE_ARRAY:
            // ARB_explicit_attrib_location is supported since GL 3.3
            out << "layout (location = " << location << ") ";
            break;
        case HdBinding::UNIFORM:
        case HdBinding::UNIFORM_ARRAY:
        case HdBinding::BINDLESS_UNIFORM:
        case HdBinding::BINDLESS_SSBO_RANGE:
            // ARB_explicit_uniform_location is supported since GL 4.3
            out << "layout (location = " << location << ") ";
            break;
        case HdBinding::TEXTURE_2D:
        case HdBinding::ARRAY_OF_TEXTURE_2D:
        case HdBinding::BINDLESS_TEXTURE_2D:
        case HdBinding::BINDLESS_ARRAY_OF_TEXTURE_2D:
        case HdBinding::TEXTURE_FIELD:
        case HdBinding::BINDLESS_TEXTURE_FIELD:
        case HdBinding::TEXTURE_UDIM_ARRAY:
        case HdBinding::BINDLESS_TEXTURE_UDIM_ARRAY:
        case HdBinding::TEXTURE_UDIM_LAYOUT:
        case HdBinding::BINDLESS_TEXTURE_UDIM_LAYOUT:
        case HdBinding::TEXTURE_PTEX_TEXEL:
        case HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL:
        case HdBinding::TEXTURE_PTEX_LAYOUT:
        case HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT:
            out << "layout (binding = "
                << lq.binding.GetTextureUnit() << ") ";
            break;
        case HdBinding::SSBO:
            out << "layout (std430, binding = " << location << ") ";
            break;
        case HdBinding::UBO:
            out << "layout (std140, binding = " << location << ") ";
            break;
        default:
            break;
        }
        return out;
    }
}

HdStGLSLProgramSharedPtr
HdSt_CodeGen::Compile(HdStResourceRegistry*const registry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // shader sources
    // geometric shader owns main()
    std::string vertexShader =
        _geometricShader->GetSource(HdShaderTokens->vertexShader);
    std::string tessControlShader =
        _geometricShader->GetSource(HdShaderTokens->tessControlShader);
    std::string tessEvalShader =
        _geometricShader->GetSource(HdShaderTokens->tessEvalShader);
    std::string geometryShader =
        _geometricShader->GetSource(HdShaderTokens->geometryShader);
    std::string fragmentShader =
        _geometricShader->GetSource(HdShaderTokens->fragmentShader);

    bool hasVS  = (!vertexShader.empty());
    bool hasTCS = (!tessControlShader.empty());
    bool hasTES = (!tessEvalShader.empty());
    bool hasGS  = (!geometryShader.empty());
    bool hasFS  = (!fragmentShader.empty());

    // create GLSL program.
    HdStGLSLProgramSharedPtr glslProgram =
        std::make_shared<HdStGLSLProgram>(HdTokens->drawingShader, registry);

    // initialize autogen source buckets
    _genHeader.str(""); _genHeaderFS.str("");
    _genCommon.str(""); _genVS.str(""); _genTCS.str(""); _genTES.str("");
    _genGS.str(""); _genFS.str(""); _genCS.str("");
    _procVS.str(""); _procTCS.str(""), _procTES.str(""), _procGS.str("");

    // GL capabilities.
    const int glslVersion = registry->GetHgi()->GetCapabilities()->
        GetShaderVersion();
    const bool bindlessTextureEnabled = registry->GetHgi()->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBindlessTextures);
    const bool bindlessBufferEnabled = registry->GetHgi()->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBindlessBuffers);
    const bool shaderDrawParametersEnabled = registry->GetHgi()->
        GetCapabilities()->IsSet(HgiDeviceCapabilitiesBitsShaderDrawParameters);
    const bool builtinBarycentricsEnabled = registry->GetHgi()->
        GetCapabilities()->IsSet(HgiDeviceCapabilitiesBitsBuiltinBarycentrics);

    if (bindlessBufferEnabled) {
        _genHeader << "#extension GL_NV_shader_buffer_load : require\n"
                   << "#extension GL_NV_gpu_shader5 : require\n";
    }
    if (bindlessTextureEnabled) {
        _genHeader << "#extension GL_ARB_bindless_texture : require\n";
    }
    if (glslVersion < 460 && shaderDrawParametersEnabled) {
        _genHeader << "#extension GL_ARB_shader_draw_parameters : require\n";
    }
    if (builtinBarycentricsEnabled) {
        _genHeaderFS <<
                "#extension GL_NV_fragment_shader_barycentric: require\n";
    }

    // Used in glslfx files to determine if it is using new/old
    // imaging system. It can also be used as API guards when
    // we need new versions of Storm shading. 
    _genCommon << "#define HD_SHADER_API " << HD_SHADER_API << "\n";

    // XXX: this is a hacky workaround for experimental support of GL 3.3
    //      the double is used in hd_dvec3 akin, so we are likely able to
    //      refactor that helper functions.
    if (glslVersion < 400) {
        _genCommon << "#define double float\n"
                   << "#define dvec2 vec2\n"
                   << "#define dvec3 vec3\n"
                   << "#define dvec4 vec4\n"
                   << "#define dmat4 mat4\n";
    }

    // XXX: this macro is still used in GlobalUniform.
    _genCommon << "#define MAT4 " <<
        HdStGLConversions::GetGLSLTypename(
            HdVtBufferSource::GetDefaultMatrixType()) << "\n";
    // a trick to tightly pack unaligned data (vec3, etc) into SSBO/UBO.
    _genCommon << _GetPackedTypeDefinitions();

    if (_materialTag == HdStMaterialTagTokens->masked) {
        _genFS << "#define HD_MATERIAL_TAG_MASKED 1\n";
    }

    // ------------------
    // Custom Buffer Bindings
    // ----------------------
    // For custom buffer bindings, more code can be generated; a full spec is
    // emitted based on the binding declaration.
    TF_FOR_ALL(binDecl, _metaData.customBindings) {
        _genCommon << "#define "
                   << binDecl->name << "_Binding " 
                   << binDecl->binding.GetLocation() << "\n";
        _genCommon << "#define HD_HAS_" << binDecl->name << " 1\n";

        // typeless binding doesn't need declaration nor accessor.
        if (binDecl->dataType.IsEmpty()) continue;

        _EmitDeclaration(_genCommon,
                     binDecl->name,
                     binDecl->dataType,
                     binDecl->binding);
        _EmitAccessor(_genCommon,
                      binDecl->name,
                      binDecl->dataType,
                      binDecl->binding,
                      (binDecl->binding.GetType() == HdBinding::UNIFORM)
                      ? NULL : "localIndex");
    }

    std::stringstream declarations;
    std::stringstream accessors;
    TF_FOR_ALL(it, _metaData.customInterleavedBindings) {
        // note: _constantData has been sorted by offset in HdSt_ResourceBinder.
        // XXX: not robust enough, should consider padding and layouting rules
        // to match with the logic in HdInterleavedMemoryManager if we
        // want to use a layouting policy other than default padding.

        HdBinding binding = it->first;
        TfToken typeName(TfStringPrintf("CustomBlockData%d", binding.GetValue()));
        TfToken varName = it->second.blockName;

        declarations << "struct " << typeName << " {\n";

        // dbIt is StructEntry { name, dataType, offset, numElements }
        TF_FOR_ALL (dbIt, it->second.entries) {
            _genCommon << "#define HD_HAS_" << dbIt->name << " 1\n";
            declarations << "  " << _GetPackedType(dbIt->dataType, false)
                         << " " << dbIt->name;
            if (dbIt->arraySize > 1) {
                _genCommon << "#define HD_NUM_" << dbIt->name
                           << " " << dbIt->arraySize << "\n";
                declarations << "[" << dbIt->arraySize << "]";
            }
            declarations <<  ";\n";

            if (it->second.arraySize > 0) {
                _EmitStructAccessor(accessors, varName, 
                                    dbIt->name, dbIt->dataType, dbIt->arraySize,
                                    "localIndex", dbIt->concatenateNames);
            } else {
                _EmitStructAccessor(accessors, varName, 
                                    dbIt->name, dbIt->dataType, dbIt->arraySize,
                                    NULL,  dbIt->concatenateNames);
            }
        }

        declarations << "};\n";
        _EmitDeclaration(declarations, varName, typeName, binding, it->second.arraySize);
    }
    _genCommon << declarations.str()
               << accessors.str();

    // HD_NUM_PATCH_VERTS, HD_NUM_PRIMTIIVE_VERTS
    if (_geometricShader->IsPrimTypePatches()) {
        _genCommon << "#define HD_NUM_PATCH_VERTS "
                   << _geometricShader->GetPrimitiveIndexSize() << "\n";
        _genCommon << "#define HD_NUM_PATCH_EVAL_VERTS "
                   << _geometricShader->GetNumPatchEvalVerts() << "\n";
    }
    _genCommon << "#define HD_NUM_PRIMITIVE_VERTS "
               << _geometricShader->GetNumPrimitiveVertsForGeometryShader()
               << "\n";

    // include ptex utility (if needed)
    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {
        HdBinding::Type bindingType = it->first.GetType();
        if (bindingType == HdBinding::TEXTURE_PTEX_TEXEL ||
            bindingType == HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL) {
            _genCommon << _GetPtexTextureShaderSource();
            break;
        }
    }

    TF_FOR_ALL (it, _metaData.topologyVisibilityData) {
        TF_FOR_ALL (pIt, it->second.entries) {
            _genCommon << "#define HD_HAS_" << pIt->name  << " 1\n";
        }
    }

    // primvar existence macros

    // XXX: this is temporary, until we implement the fallback value definition
    // for any primvars used in glslfx.
    // Note that this #define has to be considered in the hash computation
    // since it changes the source code. However we have already combined the
    // entries of instanceData into the hash value, so it's not needed to be
    // added separately, at least in current usage.
    TF_FOR_ALL (it, _metaData.constantData) {
        TF_FOR_ALL (pIt, it->second.entries) {
            _genCommon << "#define HD_HAS_" << pIt->name << " 1\n";
        }
    }
    TF_FOR_ALL (it, _metaData.instanceData) {
        _genCommon << "#define HD_HAS_INSTANCE_" << it->second.name << " 1\n";
        _genCommon << "#define HD_HAS_"
                   << it->second.name << "_" << it->second.level << " 1\n";
    }
    _genCommon << "#define HD_INSTANCER_NUM_LEVELS "
               << _metaData.instancerNumLevels << "\n"
               << "#define HD_INSTANCE_INDEX_WIDTH "
               << (_metaData.instancerNumLevels+1) << "\n"; 
    if (!_geometricShader->IsPrimTypePoints()) {
        TF_FOR_ALL (it, _metaData.elementData) {
            _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
        }
        TF_FOR_ALL (it, _metaData.fvarData) {
            _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
        }
    }
    TF_FOR_ALL (it, _metaData.vertexData) {
        _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
    }
    TF_FOR_ALL (it, _metaData.varyingData) {
        _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
    }
    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {
        // XXX: HdBinding::PRIMVAR_REDIRECT won't define an accessor if it's
        // an alias of like-to-like, so we want to suppress the HD_HAS_* flag
        // as well.

        // For PRIMVAR_REDIRECT, the HD_HAS_* flag will be defined after
        // the corresponding HdGet_* function.

        // XXX: (HYD-1882) The #define HD_HAS_... for a primvar
        // redirect will be defined immediately after the primvar
        // redirect HdGet_... in the loop over
        // _metaData.shaderParameterBinding below.  Given that this
        // loop is not running in a canonical order (e.g., textures
        // first, then primvar redirects, ...) and that the texture is
        // picking up the HD_HAS_... flag, the answer to the following
        // question is random:
        //
        // If there is a texture trying to use a primvar called NAME
        // for coordinates and there is a primvar redirect called NAME,
        // will the texture use it or not?
        // 
        HdBinding::Type bindingType = it->first.GetType();
        if (bindingType != HdBinding::PRIMVAR_REDIRECT) {
            _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
        }

        // For any texture shader parameter we also emit the texture 
        // coordinates associated with it
        if (bindingType == HdBinding::TEXTURE_2D ||
            bindingType == HdBinding::BINDLESS_TEXTURE_2D ||
            bindingType == HdBinding::ARRAY_OF_TEXTURE_2D ||
            bindingType == HdBinding::BINDLESS_ARRAY_OF_TEXTURE_2D ||
            bindingType == HdBinding::TEXTURE_UDIM_ARRAY || 
            bindingType == HdBinding::BINDLESS_TEXTURE_UDIM_ARRAY) {
            _genCommon
                << "#define HD_HAS_COORD_" << it->second.name << " 1\n";
        }
    }

    // mixin shaders
    _genCommon << _geometricShader->GetSource(HdShaderTokens->commonShaderSource);
    TF_FOR_ALL(it, _shaders) {
        _genCommon << (*it)->GetSource(HdShaderTokens->commonShaderSource);
    }

    // Needed for patch-based face-varying primvar refinement
    if (_geometricShader->GetFvarPatchType() == 
        HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE ||
        _geometricShader->GetFvarPatchType() == 
        HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
        using PatchShaderSource = OpenSubdiv::Osd::GLSLPatchShaderSource;

        if (hasGS) {
            _genGS << "#define OSD_PATCH_BASIS_GLSL\n";
            _genGS << PatchShaderSource::GetPatchBasisShaderSource();
        } else {
            _genFS << "#define OSD_PATCH_BASIS_GLSL\n";
            _genFS << PatchShaderSource::GetPatchBasisShaderSource();
        }
    }

    // Barycentric coordinates
    if (builtinBarycentricsEnabled) {
        _genFS << "vec3 GetBarycentricCoord() {\n"
                  "  return gl_BaryCoordNoPerspNV;\n"
                  "}\n";
    } else {
        if (hasGS) {
            _genGS << "noperspective out vec3 hd_barycentricCoord;\n";
            _genFS << "noperspective in vec3 hd_barycentricCoord;\n"
                      "vec3 GetBarycentricCoord() {\n"
                      "  return hd_barycentricCoord;\n"
                      "}\n";
        } else {
            _genFS << "vec3 GetBarycentricCoord() {\n"
                      "  return vec3(0);\n"
                      "}\n";
        }
    }

    // prep interstage plumbing function
    _procVS  << "void ProcessPrimvars() {\n";
    _procTCS << "void ProcessPrimvars() {\n";
    _procTES << "float ProcessPrimvar(float inPv0, float inPv1, float inPv2, float inPv3, vec4 basis, vec2 uv);\n";
    _procTES << "vec2 ProcessPrimvar(vec2 inPv0, vec2 inPv1, vec2 inPv2, vec2 inPv3, vec4 basis, vec2 uv);\n";
    _procTES << "vec3 ProcessPrimvar(vec3 inPv0, vec3 inPv1, vec3 inPv2, vec3 inPv3, vec4 basis, vec2 uv);\n";
    _procTES << "vec4 ProcessPrimvar(vec4 inPv0, vec4 inPv1, vec4 inPv3, vec4 inPv3, vec4 basis, vec2 uv);\n";
    _procTES << "void ProcessPrimvars(vec4 basis, int i0, int i1, int i2, int i3, vec2 uv) {\n";
    // geometry shader plumbing
    switch(_geometricShader->GetPrimitiveType())
    {
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIQUADS:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_QUADS:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIQUADS:
        {
            _procGS << "vec4 GetPatchCoord(int index);\n"
                    << "void ProcessPrimvars(int index) {\n"
                    << "  vec2 localST = GetPatchCoord(index).xy;\n";
            break;
        }
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BSPLINE:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
        {
            _procGS << "void ProcessPrimvars(int index, vec2 tessST) {\n"
                    << "  vec2 localST = tessST;\n";
            break;
        }
        default: // points, basis curves
            // do nothing. no additional code needs to be generated.
            ;
    }
    if (!builtinBarycentricsEnabled) {
        switch(_geometricShader->GetPrimitiveType())
        {
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_QUADS:
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_QUADS:
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BSPLINE:
            {
                // These correspond to built-in fragment shader barycentric
                // coords except reversed for the second triangle in the quad.
                // Each quad is split into two triangles with indices (3,0,2)
                // and (1,2,0).
                _procGS << "  const vec3 coords[4] = vec3[](\n"
                        << "   vec3(0,0,1), vec3(1,0,0), "
                        << "vec3(0,1,0), vec3(1,0,0)\n"
                        << "  );\n"
                        << "  hd_barycentricCoord = coords[index];\n";
                break;
            }
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIQUADS:
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIQUADS:
            case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
            {
                // These correspond to built-in fragment shader barycentric
                // coords.
                _procGS << "  const vec3 coords[3] = vec3[](\n"
                        << "   vec3(1,0,0), vec3(0,1,0), vec3(0,0,1)\n"
                        << "  );\n"
                        << "  hd_barycentricCoord = coords[index];\n";
                break;
            }
            default: // points, basis curves
                // do nothing. no additional code needs to be generated.
                ;
        }
    }

    // generate drawing coord and accessors
    _GenerateDrawingCoord();

    // generate primvars
    _GenerateConstantPrimvar();
    _GenerateInstancePrimvar();
    _GenerateElementPrimvar();
    _GenerateVertexAndFaceVaryingPrimvar(hasGS, shaderDrawParametersEnabled, glslVersion);

    _GenerateTopologyVisibilityParameters();

    //generate shader parameters (is going last since it has primvar redirects)
    _GenerateShaderParameters(bindlessTextureEnabled);

    // finalize buckets
    _procVS  << "}\n";
    _procGS  << "}\n";
    _procTCS << "}\n";
    _procTES << "}\n";

    // insert interstage primvar plumbing procs into genVS/TCS/TES/GS
    _genVS  << _procVS.str();
    _genTCS << _procTCS.str();
    _genTES << _procTES.str();
    _genGS  << _procGS.str();


    // other shaders (renderpass, lighting, surface) first
    TF_FOR_ALL(it, _shaders) {
        HdStShaderCodeSharedPtr const &shader = *it;
        if (hasVS)
            _genVS  << shader->GetSource(HdShaderTokens->vertexShader);
        if (hasTCS)
            _genTCS << shader->GetSource(HdShaderTokens->tessControlShader);
        if (hasTES)
            _genTES << shader->GetSource(HdShaderTokens->tessEvalShader);
        if (hasGS)
            _genGS  << shader->GetSource(HdShaderTokens->geometryShader);
        if (hasFS)
            _genFS  << shader->GetSource(HdShaderTokens->fragmentShader);
    }

    // OpenSubdiv tessellation shader (if required)
    if (tessControlShader.find("OsdPerPatchVertexBezier") != std::string::npos) {
        _genTCS << OpenSubdiv::Osd::GLSLPatchShaderSource::GetCommonShaderSource();
        _genTCS << "MAT4 GetWorldToViewMatrix();\n";
        _genTCS << "MAT4 GetProjectionMatrix();\n";
        _genTCS << "float GetTessLevel();\n";
        // we apply modelview in the vertex shader, so the osd shaders doesn't need
        // to apply again.
        _genTCS << "mat4 OsdModelViewMatrix() { return mat4(1); }\n";
        _genTCS << "mat4 OsdProjectionMatrix() { return mat4(GetProjectionMatrix()); }\n";
        _genTCS << "int OsdPrimitiveIdBase() { return 0; }\n";
        _genTCS << "float OsdTessLevel() { return GetTessLevel(); }\n";
    }
    if (tessEvalShader.find("OsdPerPatchVertexBezier") != std::string::npos) {
        _genTES << OpenSubdiv::Osd::GLSLPatchShaderSource::GetCommonShaderSource();
        _genTES << "mat4 OsdModelViewMatrix() { return mat4(1); }\n";
    }
    if (geometryShader.find("OsdInterpolatePatchCoord") != std::string::npos) {
        _genGS <<  OpenSubdiv::Osd::GLSLPatchShaderSource::GetCommonShaderSource();
    }
    if (fragmentShader.find("OsdInterpolatePatchCoord") != std::string::npos) {
        _genFS <<  OpenSubdiv::Osd::GLSLPatchShaderSource::GetCommonShaderSource();
    }

    // geometric shader
    _genVS  << vertexShader;
    _genTCS << tessControlShader;
    _genTES << tessEvalShader;
    _genGS  << geometryShader;
    _genFS  << fragmentShader;

    // Sanity check that if you provide a control shader, you have also provided
    // an evaluation shader (and vice versa)
    if (hasTCS ^ hasTES) {
        TF_CODING_ERROR(
            "tessControlShader and tessEvalShader must be provided together.");
        hasTCS = hasTES = false;
    };

    bool shaderCompiled = false;
    // compile shaders
    // note: _vsSource, _fsSource etc are used for diagnostics (see header)
    if (hasVS) {
        _vsSource = _genHeader.str() + _genCommon.str() + _genVS.str();
        if (!glslProgram->CompileShader(HgiShaderStageVertex, _vsSource)) {
            return HdStGLSLProgramSharedPtr();
        }
        shaderCompiled = true;
    }
    if (hasFS) {
        _fsSource = _genHeader.str() + _genHeaderFS.str() +
                    _genCommon.str() + _genFS.str();
        if (!glslProgram->CompileShader(HgiShaderStageFragment, _fsSource)) {
            return HdStGLSLProgramSharedPtr();
        }
        shaderCompiled = true;
    }
    if (hasTCS) {
        _tcsSource = _genHeader.str() + _genCommon.str() + _genTCS.str();
        if (!glslProgram->CompileShader(
                HgiShaderStageTessellationControl, _tcsSource)) {
            return HdStGLSLProgramSharedPtr();
        }
        shaderCompiled = true;
    }
    if (hasTES) {
        _tesSource = _genHeader.str() + _genCommon.str() + _genTES.str();
        if (!glslProgram->CompileShader(
                HgiShaderStageTessellationEval, _tesSource)) {
            return HdStGLSLProgramSharedPtr();
        }
        shaderCompiled = true;
    }
    if (hasGS) {
        _gsSource = _genHeader.str() + _genCommon.str() + _genGS.str();
        if (!glslProgram->CompileShader(HgiShaderStageGeometry, _gsSource)) {
            return HdStGLSLProgramSharedPtr();
        }
        shaderCompiled = true;
    }

    if (!shaderCompiled) {
        return HdStGLSLProgramSharedPtr();
    }

    return glslProgram;
}

HdStGLSLProgramSharedPtr
HdSt_CodeGen::CompileComputeProgram(HdStResourceRegistry*const registry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // initialize autogen source buckets
    _genCommon.str(""); _genVS.str(""); _genTCS.str(""); _genTES.str("");
    _genGS.str(""); _genFS.str(""); _genCS.str("");
    _procVS.str(""); _procTCS.str(""), _procTES.str(""), _procGS.str("");
    
    // GL capabilities 
    const bool bindlessBufferEnabled = registry->GetHgi()->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBindlessBuffers);
    const bool bindlessTextureEnabled = registry->GetHgi()->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBindlessTextures);
    
    if (bindlessBufferEnabled) {
        _genCommon << "#extension GL_NV_shader_buffer_load : require\n"
                   << "#extension GL_NV_gpu_shader5 : require\n";
    }
    if (bindlessTextureEnabled) {
        _genCommon << "#extension GL_ARB_bindless_texture : require\n";
    }

    // Used in glslfx files to determine if it is using new/old
    // imaging system. It can also be used as API guards when
    // we need new versions of Storm shading. 
    _genCommon << "#define HD_SHADER_API " << HD_SHADER_API << "\n";    

    // a trick to tightly pack unaligned data (vec3, etc) into SSBO/UBO.
    _genCommon << _GetPackedTypeDefinitions();
    
    std::stringstream uniforms;
    std::stringstream declarations;
    std::stringstream accessors;
    
    uniforms << "// Uniform block\n";

    HdBinding uboBinding(HdBinding::UBO, 0);
    uniforms << pxrImagingHdStCodeGen::LayoutQualifier(uboBinding);
    uniforms << "uniform ubo_" << uboBinding.GetLocation() << " {\n";

    accessors << "// Read-Write Accessors & Mutators\n";
    uniforms << "    int vertexOffset;       // offset in aggregated buffer\n";
    TF_FOR_ALL(it, _metaData.computeReadWriteData) {
        TfToken const &name = it->second.name;
        HdBinding const &binding = it->first;
        TfToken const &dataType = it->second.dataType;

        // For now, SSBO bindings use a flat type encoding.
        TfToken declDataType =
            (binding.GetType() == HdBinding::SSBO
                ? _GetFlatType(dataType) : dataType);
        
        uniforms << "    int " << name << "Offset;\n";
        uniforms << "    int " << name << "Stride;\n";
        
        _EmitDeclaration(declarations,
                name,
                declDataType,
                binding, 0);
        // getter & setter
        {
            std::stringstream indexing;
            indexing << "(localIndex + vertexOffset)"
                     << " * " << name << "Stride"
                     << " + " << name << "Offset";
            _EmitComputeAccessor(accessors, name, dataType, binding,
                    indexing.str().c_str());
            _EmitComputeMutator(accessors, name, dataType, binding,
                    indexing.str().c_str());
        }
    }
    accessors << "// Read-Only Accessors\n";
    // no vertex offset for constant data
    TF_FOR_ALL(it, _metaData.computeReadOnlyData) {
        TfToken const &name = it->second.name;
        HdBinding const &binding = it->first;
        TfToken const &dataType = it->second.dataType;
        
        // For now, SSBO bindings use a flat type encoding.
        TfToken declDataType =
            (binding.GetType() == HdBinding::SSBO
                ? _GetFlatType(dataType) : dataType);

        uniforms << "    int " << name << "Offset;\n";
        uniforms << "    int " << name << "Stride;\n";

        _EmitDeclaration(declarations,
                name,
                declDataType,
                binding, 0);
        // getter
        {
            std::stringstream indexing;
            // no vertex offset for constant data
            indexing << "(localIndex)"
                     << " * " << name << "Stride"
                     << " + " << name << "Offset";
            _EmitComputeAccessor(accessors, name, dataType, binding,
                    indexing.str().c_str());
        }
    }
    uniforms << "};\n";
    
    _genCommon << uniforms.str()
               << declarations.str()
               << accessors.str();
    
    // other shaders (renderpass, lighting, surface) first
    TF_FOR_ALL(it, _shaders) {
        HdStShaderCodeSharedPtr const &shader = *it;
        _genCS  << shader->GetSource(HdShaderTokens->computeShader);
    }

    // main
    _genCS << "void main() {\n";
    _genCS << "  int computeCoordinate = int(gl_GlobalInvocationID.x);\n";
    _genCS << "  compute(computeCoordinate);\n";
    _genCS << "}\n";
    
    // create GLSL program.
    HdStGLSLProgramSharedPtr glslProgram =
        std::make_shared<HdStGLSLProgram>(HdTokens->computeShader, registry);
    
    // compile shaders
    {
        _csSource = _genCommon.str() + _genCS.str();
        if (!glslProgram->CompileShader(HgiShaderStageCompute, _csSource)) {
            return HdStGLSLProgramSharedPtr();
        }
    }
    
    return glslProgram;
}

static void _EmitDeclaration(std::stringstream &str,
                             TfToken const &name,
                             TfToken const &type,
                             HdBinding const &binding,
                             int arraySize)
{
    /*
      [vertex attribute]
         layout (location = <location>) in <type> <name>;
      [uniform]
         layout (location = <location>) uniform <type> <name>;
      [SSBO]
         layout (std430, binding = <location>) buffer buffer_<location> {
            <type> <name>[];
         };
      [Bindless Uniform]
         layout (location = <location>) uniform <type> *<name>;

     */
    HdBinding::Type bindingType = binding.GetType();

    if (!TF_VERIFY(!name.IsEmpty())) return;
    if (!TF_VERIFY(!type.IsEmpty(),
                      "Unknown dataType for %s",
                      name.GetText())) return;

    if (arraySize > 0) {
        if (!TF_VERIFY(bindingType == HdBinding::UNIFORM_ARRAY                ||
                          bindingType == HdBinding::DRAW_INDEX_INSTANCE_ARRAY ||
                          bindingType == HdBinding::UBO                       ||
                          bindingType == HdBinding::SSBO                      ||
                          bindingType == HdBinding::BINDLESS_SSBO_RANGE       ||
                          bindingType == HdBinding::BINDLESS_UNIFORM)) {
            // XXX: SSBO and BINDLESS_UNIFORM don't need arraySize, but for the
            // workaround of UBO allocation we're passing arraySize = 2
            // for all bindingType.
            return;
        }
    }

    // layout qualifier (if exists)
    str << pxrImagingHdStCodeGen::LayoutQualifier(binding);

    switch (bindingType) {
    case HdBinding::VERTEX_ATTR:
    case HdBinding::DRAW_INDEX:
    case HdBinding::DRAW_INDEX_INSTANCE:
        str << "in " << _GetPackedType(type, false) << " " << name << ";\n";
        break;
    case HdBinding::DRAW_INDEX_INSTANCE_ARRAY:
        str << "in " << _GetPackedType(type, false) << " " << name
            << "[" << arraySize << "];\n";
        break;
    case HdBinding::UNIFORM:
        str << "uniform " << _GetPackedType(type, false) << " " << name << ";\n";
        break;
    case HdBinding::UNIFORM_ARRAY:
        str << "uniform " << _GetPackedType(type, false) << " " << name
            << "[" << arraySize << "];\n";
        break;
    case HdBinding::UBO:
        str << "uniform ubo_" << name <<  " {\n"
            << "  " << _GetPackedType(type, true)
            << " " << name;
        if (arraySize > 0) {
            str << "[" << arraySize << "];\n";
        } else {
            str << ";\n";
        }
        str << "};\n";
        break;
    case HdBinding::SSBO:
        str << "buffer buffer_" << binding.GetLocation() << " {\n"
            << "  " << _GetPackedType(type, true)
            << " " << name << "[];\n"
            << "};\n";
        break;
    case HdBinding::BINDLESS_SSBO_RANGE:
        str << "uniform " << _GetPackedType(type, true)
            << " *" << name << ";\n";
        break;
    case HdBinding::BINDLESS_UNIFORM:
        str << "uniform " << _GetPackedType(type, true)
            << " *" << name << ";\n";
        break;
    default:
        TF_CODING_ERROR("Unknown binding type %d, for %s\n",
                        binding.GetType(), name.GetText());
        break;
    }
}

static void _EmitDeclaration(
    std::stringstream &str,
    HdSt_ResourceBinder::MetaData::BindingDeclaration const &bindingDeclaration,
    int arraySize=0)
{
    _EmitDeclaration(str,
         bindingDeclaration.name,
         bindingDeclaration.dataType,
         bindingDeclaration.binding,
         arraySize);
}

static void _EmitStructAccessor(std::stringstream &str,
                                TfToken const &structName,
                                TfToken const &name,
                                TfToken const &type,
                                int arraySize,
                                const char *index,
                                bool concatenateNames)
{
    // index != NULL  if the struct is an array
    // arraySize > 1  if the struct entry is an array.
    TfToken accessorName = concatenateNames ? 
        TfToken(structName.GetString() + "_" + name.GetString()) : name;
    if (index) {
        if (arraySize > 1) {
            str << _GetUnpackedType(type, false) << " HdGet_" << accessorName
                << "(int arrayIndex, int localIndex) {\n"
                // storing to a local variable to avoid the nvidia-driver
                // bug #1561110 (fixed in 346.59)
                << "  int index = " << index << ";\n"
                << "  return "
                << _GetPackedTypeAccessor(type, false) << "("
                << structName << "[index]." << name << "[arrayIndex]);\n}\n";
        } else {
            str << _GetUnpackedType(type, false) << " HdGet_" << accessorName
                << "(int localIndex) {\n"
                << "  int index = " << index << ";\n"
                << "  return "
                << _GetPackedTypeAccessor(type, false) << "("
                << structName << "[index]." << name << ");\n}\n";
        }
    } else {
        if (arraySize > 1) {
            str << _GetUnpackedType(type, false) << " HdGet_" << accessorName
                << "(int arrayIndex, int localIndex) { return "
                << _GetPackedTypeAccessor(type, false) << "("
                << structName << "." << name << "[arrayIndex]);}\n";
        } else {
            str << _GetUnpackedType(type, false) << " HdGet_" << accessorName
                << "(int localIndex) { return "
                << _GetPackedTypeAccessor(type, false) << "("
                << structName << "." << name << ");}\n";
        }
    }
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to localIndex=0
    if (arraySize > 1) {
        str << _GetUnpackedType(type, false) << " HdGet_" << accessorName
            << "(int arrayIndex)"
            << " { return HdGet_" << accessorName << "(arrayIndex, 0); }\n";
    } else {
        str << _GetUnpackedType(type, false) << " HdGet_" << accessorName << "()"
            << " { return HdGet_" << accessorName << "(0); }\n";
    }
}

static void _EmitBufferAccessor(std::stringstream &str,
                                TfToken const &name,
                                TfToken const &type,
                                const char *index)
{
    if (index) {
        str << _GetUnpackedType(type, false) << " HdGet_" << name
            << "(int localIndex) {\n"
            << "  int index = " << index << ";\n"
            << "  return "
                << _GetPackedTypeAccessor(type, true) << "("
            << name << "[index]);\n}\n";
    } 
    str << _GetUnpackedType(type, false) << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";
}

static std::string _GetSwizzleString(TfToken const& type, 
                                     std::string const& swizzle=std::string())
{
    if (!swizzle.empty()) {
        return "." + swizzle;
    } 
    if (type == pxrImagingHdStCodeGen::_tokens->vec4 || type == pxrImagingHdStCodeGen::_tokens->ivec4) {
        return "";
    }
    if (type == pxrImagingHdStCodeGen::_tokens->vec3 || type == pxrImagingHdStCodeGen::_tokens->ivec3) {
        return ".xyz";
    }
    if (type == pxrImagingHdStCodeGen::_tokens->vec2 || type == pxrImagingHdStCodeGen::_tokens->ivec2) {
        return ".xy";
    }
    if (type == pxrImagingHdStCodeGen::_tokens->_float || type == pxrImagingHdStCodeGen::_tokens->_int) {
        return ".x";
    }
    if (type == pxrImagingHdStCodeGen::_tokens->packed_2_10_10_10) {            
        return ".x";
    }

    return "";
}

static int _GetNumComponents(TfToken const& type)
{
    int numComponents = 1;
    if (type == pxrImagingHdStCodeGen::_tokens->vec2 || type == pxrImagingHdStCodeGen::_tokens->ivec2) {
        numComponents = 2;
    } else if (type == pxrImagingHdStCodeGen::_tokens->vec3 || type == pxrImagingHdStCodeGen::_tokens->ivec3) {
        numComponents = 3;
    } else if (type == pxrImagingHdStCodeGen::_tokens->vec4 || type == pxrImagingHdStCodeGen::_tokens->ivec4) {
        numComponents = 4;
    } else if (type == pxrImagingHdStCodeGen::_tokens->mat3 || type == pxrImagingHdStCodeGen::_tokens->dmat3) {
        numComponents = 9;
    } else if (type == pxrImagingHdStCodeGen::_tokens->mat4 || type == pxrImagingHdStCodeGen::_tokens->dmat4) {
        numComponents = 16;
    }

    return numComponents;
}

static void _EmitComputeAccessor(
                    std::stringstream &str,
                    TfToken const &name,
                    TfToken const &type,
                    HdBinding const &binding,
                    const char *index)
{
    if (index) {
        str << _GetUnpackedType(type, false)
            << " HdGet_" << name << "(int localIndex) {\n";
        if (binding.GetType() == HdBinding::SSBO) {
            str << "  int index = " << index << ";\n";
            str << "  return " << _GetPackedTypeAccessor(type, false) << "("
                << _GetPackedType(type, false) << "(";
            int numComponents = _GetNumComponents(type);
            for (int c = 0; c < numComponents; ++c) {
                if (c > 0) {
                    str << ",\n              ";
                }
                str << name << "[index + " << c << "]";
            }
            str << "));\n}\n";
        } else if (binding.GetType() == HdBinding::BINDLESS_SSBO_RANGE) {
            str << "  return " << _GetPackedTypeAccessor(type, true) << "("
                << name << "[localIndex]);\n}\n";
        } else {
            str << "  return " << _GetPackedTypeAccessor(type, true) << "("
                << name << "[localIndex]);\n}\n";
        }
    } else {
        // non-indexed, only makes sense for uniform or vertex.
        if (binding.GetType() == HdBinding::UNIFORM || 
            binding.GetType() == HdBinding::VERTEX_ATTR) {
            str << _GetUnpackedType(type, false)
                << " HdGet_" << name << "(int localIndex) { return ";
            str << _GetPackedTypeAccessor(type, true) << "(" << name << ");}\n";
        }
    }
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to locaIndex=0
    str << _GetUnpackedType(type, false) << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";
    
}

static void _EmitComputeMutator(
                    std::stringstream &str,
                    TfToken const &name,
                    TfToken const &type,
                    HdBinding const &binding,
                    const char *index)
{
    if (index) {
        str << "void"
            << " HdSet_" << name << "(int localIndex, "
            << _GetUnpackedType(type, false) << " value) {\n";
        if (binding.GetType() == HdBinding::SSBO) {
            str << "  int index = " << index << ";\n";
            str << "  " << _GetPackedType(type, false) << " packedValue = "
                << _GetPackedTypeMutator(type, false) << "(value);\n";
            int numComponents = _GetNumComponents(_GetPackedType(type, false));
            if (numComponents == 1) {
                str << "  "
                    << name << "[index] = packedValue;\n";
            } else {
                for (int c = 0; c < numComponents; ++c) {
                    str << "  "
                        << name << "[index + " << c << "] = "
                        << "packedValue[" << c << "];\n";
                }
            }
        } else if (binding.GetType() == HdBinding::BINDLESS_SSBO_RANGE) {
            str << name << "[localIndex] = "
                << _GetPackedTypeMutator(type, true) << "(value);\n";
        } else {
            TF_WARN("mutating non-SSBO not supported");
        }
        str << "}\n";
    } else {
        TF_WARN("mutating non-indexed data not supported");
    }
    // XXX Don't output a default mutator as we don't want accidental overwrites
    // of compute read-write data.
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to locaIndex=0
    //str << "void HdSet_" << name << "(" << type << " value)"
    //    << " { HdSet_" << name << "(0, value); }\n";
    
}

static void _EmitAccessor(std::stringstream &str,
                          TfToken const &name,
                          TfToken const &type,
                          HdBinding const &binding,
                          const char *index)
{
    if (index) {
        str << _GetUnpackedType(type, false)
            << " HdGet_" << name << "(int localIndex) {\n"
            << "  int index = " << index << ";\n"
            << "  return " << _GetPackedTypeAccessor(type, true) << "("
            << name << "[index]);\n}\n";
    } else {
        // non-indexed, only makes sense for uniform or vertex.
        if (binding.GetType() == HdBinding::UNIFORM || 
            binding.GetType() == HdBinding::VERTEX_ATTR) {
            str << _GetUnpackedType(type, false)
                << " HdGet_" << name << "(int localIndex) { return ";
            str << _GetPackedTypeAccessor(type, true) << "(" << name << ");}\n";
        }
    }
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to locaIndex=0
    str << _GetUnpackedType(type, false) << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";
    
}

static void _EmitTextureAccessors(
    std::stringstream &accessors,
    HdSt_ResourceBinder::MetaData::ShaderParameterAccessor const &acc,
    std::string const &swizzle,
    int const dim,
    bool const hasTextureTransform,
    bool const hasTextureScaleAndBias,
    bool const isBindless,
    bool const bindlessTextureEnabled,
    bool const isArray=false,
    bool const isShadowSampler=false)
{
    TfToken const &name = acc.name;

    int const coordDim = isShadowSampler ? dim + 1 : dim;
    std::string const samplerType = isShadowSampler ? 
        "sampler" + std::to_string(dim) + "DShadow" : 
        "sampler" + std::to_string(dim) + "D";

    // Forward declare texture scale and bias
    if (hasTextureScaleAndBias) {
        accessors 
            << "#ifdef HD_HAS_" << name << "_" << HdStTokens->scale << "\n"
            << "vec4 HdGet_" << name << "_" << HdStTokens->scale  << "();\n"
            << "#endif\n"
            << "#ifdef HD_HAS_" << name << "_" << HdStTokens->bias  << "\n"
            << "vec4 HdGet_" << name << "_" << HdStTokens->bias  << "();\n"
            << "#endif\n";
    }

    if (!isBindless) {
        // a function returning sampler requires bindless_texture
        if (bindlessTextureEnabled) {
            if (isArray) {
                accessors
                    << samplerType << " "
                    << "HdGetSampler_" << name << "(int index) {\n"
                    << "  return sampler" << dim << "d_" << name << "[index];\n"
                    << "}\n";
            } else {
                accessors
                    << samplerType << " "
                    << "HdGetSampler_" << name << "() {\n"
                    << "  return sampler" << dim << "d_" << name << ";\n"
                    << "}\n";
            }
        } else {
            if (isArray) {
                accessors
                    << samplerType << " HdGetSampler_" << name 
                    << "(int index) {\n"
                    << " return sampler" << dim << "d_" << name 
                    << "[index];\n}\n";
            } else {
                accessors
                    << samplerType << " HdGetSampler_" << name << "() {\n"
                    << " return sampler" << dim << "d_" << name << ";\n}\n";
            }
        }
    } else {
        if (bindlessTextureEnabled) {
            if (isArray) {
                accessors
                    << samplerType << " "
                    << "HdGetSampler_" << name << "(int index) {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  return " << samplerType << "("
                    << "    shaderData[shaderCoord]." << name << ");\n"
                    << "}\n";
            } else {
                accessors
                    << samplerType << " "
                    << "HdGetSampler_" << name << "() {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  return " << samplerType << "("
                    << "    shaderData[shaderCoord]." << name << ");\n"
                    << "}\n";
            }
        }
    }

    TfToken const &dataType = acc.dataType;

    if (hasTextureTransform) {
        // Declare an eye to sampling transform and define function
        // to initialize it.
        const std::string eyeToSamplingTransform =
            "eyeTo" + name.GetString() + "SamplingTransform";

        // Computations in eye space are done with float precision, so the
        // eye to sampling transform is mat4.
        // Note that the multiplication that yiels this sampling transform
        // might be done using higher precision.
        accessors
            << "mat4 " << eyeToSamplingTransform << ";\n"
            << "\n"
            << "void Process_" << eyeToSamplingTransform
            << "(MAT4 instanceModelViewInverse) { \n"
            << "    int shaderCoord = GetDrawingCoord().shaderCoord; \n"
            << "    " << eyeToSamplingTransform << " = mat4(\n"
            << "        MAT4(shaderData[shaderCoord]."
            << name << HdSt_ResourceBindingSuffixTokens->samplingTransform
            << ") * instanceModelViewInverse);\n"
            << "}\n";
    }

    if (isArray) {
        accessors
        << _GetUnpackedType(dataType, false)
        << " HdGet_" << name << "(int index, vec" << coordDim << " coord) {\n"
        << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n";
    } else {
        accessors
            << _GetUnpackedType(dataType, false)
            << " HdGet_" << name << "(vec" << coordDim << " coord) {\n"
            << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n";
    }

    if (hasTextureTransform) {
        const std::string eyeToSamplingTransform =
            "eyeTo" + name.GetString() + "SamplingTransform";

        accessors
            << "   vec4 c = " << eyeToSamplingTransform
            << " * vec4(coord, 1);\n"
            << "   vec3 sampleCoord = c.xyz / c.w;\n";
    } else {
        accessors
            << "  vec" << coordDim << " sampleCoord = coord;\n";
    }

    if (hasTextureScaleAndBias) {
        accessors
            << "  " << _GetUnpackedType(dataType, false)
            << " result = "
            << _GetPackedTypeAccessor(dataType, false)
            << "((texture(HdGetSampler_" << name;
        if (isArray) {
            accessors << "(index), sampleCoord)\n";
        } else {
            accessors << "(), sampleCoord)\n";
        }
        accessors
            << "#ifdef HD_HAS_" << name << "_" << HdStTokens->scale << "\n"
            << "    * HdGet_" << name << "_" << HdStTokens->scale << "()\n"
            << "#endif\n" 
            << "#ifdef HD_HAS_" << name << "_" << HdStTokens->bias << "\n"
            << "    + HdGet_" << name << "_" << HdStTokens->bias  << "()\n"
            << "#endif\n"
            << ")" << swizzle << ");\n";
    } else {
        accessors
            << "  " << _GetUnpackedType(dataType, false)
            << " result = "
            << _GetPackedTypeAccessor(dataType, false)
            << "(texture(HdGetSampler_" << name;
        if (isArray) {
            accessors << "(index), sampleCoord)";
        } else {
            accessors << "(), sampleCoord)\n";
        }
        accessors << swizzle << ");\n";
    }

    if (acc.processTextureFallbackValue) {
        // Check whether texture is valid (using NAME_valid)
        //
        // Note that the OpenGL standard says that the
        // implicit derivatives (for accessing the right
        // mip-level) are undefined if the texture look-up
        // happens in a non-uniform control block, thus the
        // texture lookup is unconditionally assigned to
        // result outside of the if-block.
        //
        if (isBindless) {
            accessors
                << "  if (shaderData[shaderCoord]." << name
                << " != uvec2(0, 0)) {\n";
        } else {
            accessors
                << "  if (shaderData[shaderCoord]." << name
                << HdSt_ResourceBindingSuffixTokens->valid
                << ") {\n";
        }

        if (hasTextureScaleAndBias) {
            accessors
                << "    return result;\n"
                << "  } else {\n"
                << "    return ("
                << _GetPackedTypeAccessor(dataType, false)
                << "(shaderData[shaderCoord]."
                << name
                << HdSt_ResourceBindingSuffixTokens->fallback << ")\n"
                << "#ifdef HD_HAS_" << name << "_" << HdStTokens->scale << "\n"
                << "        * HdGet_" << name << "_" << HdStTokens->scale 
                << "()" << swizzle << "\n"
                << "#endif\n" 
                << "#ifdef HD_HAS_" << name << "_" << HdStTokens->bias << "\n"
                << "        + HdGet_" << name << "_" << HdStTokens->bias 
                << "()" << swizzle << "\n"
                << "#endif\n"
                << ");\n"
                << "  }\n";
        } else {
            accessors
                << "    return result;\n"
                << "  } else {\n"
                << "    return "
                << _GetPackedTypeAccessor(dataType, false)
                << "(shaderData[shaderCoord]."
                << name
                << HdSt_ResourceBindingSuffixTokens->fallback << ");\n"
                << "  }\n";
        }
    } else {
        accessors
            << "  return result;\n";
    }
    
    accessors
        << "}\n";
    
    TfTokenVector const &inPrimvars = acc.inPrimvars;

    // Forward declare getter for inPrimvars in case it's a transform2d
    if (!inPrimvars.empty()) {
        accessors
            << "#if defined(HD_HAS_" << inPrimvars[0] << ")\n"
            << "vec" << dim << " HdGet_" << inPrimvars[0] << "(int localIndex);\n"
            << "#endif\n";
    }

    // Create accessor for texture coordinates based on texture param name
    // vec2 HdGetCoord_name(int localIndex)
    accessors
        << "vec" << coordDim << " HdGetCoord_" << name << "(int localIndex) {\n"
        << "  return \n";
    if (!inPrimvars.empty()) {
        accessors 
            << "#if defined(HD_HAS_" << inPrimvars[0] <<")\n"
            << "  HdGet_" << inPrimvars[0] << "(localIndex).xy\n"
            << "#else\n"
            << "  vec" << coordDim << "(0.0)\n"
            << "#endif\n";
    } else {
        accessors
            << "  vec" << coordDim << "(0.0)";
    }
    accessors << ";\n}\n"; 

    // vec2 HdGetCoord_name()
    accessors
        << "vec" << coordDim << " HdGetCoord_" << name << "() {"
        << "  return HdGetCoord_" << name << "(0);\n }\n";

    // vec4 HdGet_name(int localIndex)
    if (isArray) {
        accessors
            << _GetUnpackedType(dataType, false)
            << " HdGet_" << name
            << "(int localIndex) { return HdGet_" << name << "(localIndex, "
            << "HdGetCoord_" << name << "(localIndex));\n}\n";
    } else {
        accessors
            << _GetUnpackedType(dataType, false)
            << " HdGet_" << name
            << "(int localIndex) { return HdGet_" << name << "("
            << "HdGetCoord_" << name << "(localIndex));\n}\n";
    }

    // vec4 HdGet_name()
    accessors
        << _GetUnpackedType(dataType, false)
        << " HdGet_" << name
        << "() { return HdGet_" << name << "(0);\n }\n";

    // Emit pre-multiplication by alpha indicator
    if (acc.isPremultiplied) {
        accessors << "#define " << name << "_IS_PREMULTIPLIED 1\n";
    }      
}

// Accessing face varying primvar data from the GS or FS requires special
// case handling for refinement while providing a branchless solution.
// When dealing with vertices on a refined face when the face-varying data has 
// not been refined, we use the patch coord to get its parametrization on the 
// sanitized (coarse) "ptex" face, and interpolate based on the face primitive 
// type (bilinear for quad faces, barycentric for tri faces).
// When face varying data has been refined and the fvar patch type is quad or 
// tri, we still use bilinear or barycentric interpolation, respectively, but
// we do it over the refined face and use refined face-varying values, accessed
// using the refined face-varying indices.
// When the fvar patch type is b-spline or box-spline, we solve over 16 or 12
// refined values, respectively, also accessed via the refined indices, getting 
// the weights from OsdEvaluatePatchBasisNormalized(). 
static void _EmitFVarAccessor(
                bool hasGS,
                std::stringstream &str,
                TfToken const &name,
                TfToken const &type,
                HdBinding const &binding,
                HdSt_GeometricShader::PrimitiveType const& primType,
                HdSt_GeometricShader::FvarPatchType const& fvarPatchType,
                int fvarChannel)
{
    // emit an internal getter for accessing the coarse fvar data (corresponding
    // to the refined face, in the case of refinement)
    str << _GetUnpackedType(type, false)
        << " HdGet_" << name << "_Coarse(int localIndex) {\n";
    if ((fvarPatchType == 
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_QUADS) ||
        (fvarPatchType == 
            HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES)) {
        str << "  int fvarIndex = GetFVarIndex(localIndex);\n";
    } else {
        str << "  int fvarIndex = GetDrawingCoord().fvarCoord + localIndex;\n";
    }
    str << "  return " << _GetPackedTypeAccessor(type, true) << "("
        <<       name << "[fvarIndex]);\n}\n";

    // emit the (public) accessor for the fvar data, accounting for refinement
    // interpolation
    str << _GetUnpackedType(type, false)
        << " HdGet_" << name << "(int localIndex, vec2 st) {\n";

    if (fvarPatchType == 
        HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE) {
        str << "  int patchType = OSD_PATCH_DESCRIPTOR_REGULAR;\n";
    } else if (fvarPatchType == 
        HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
        str << "  int patchType = OSD_PATCH_DESCRIPTOR_LOOP;\n";
    }

    switch (fvarPatchType) {
        case HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_QUADS: 
        {
            // linear interpolation within a quad.
            str << "  return mix("
                << "mix(" << "HdGet_" << name << "_Coarse(0),"
                <<           "HdGet_" << name << "_Coarse(1), st.x),"
                << "mix(" << "HdGet_" << name << "_Coarse(3),"
                <<           "HdGet_" << name << "_Coarse(2), st.x), "
                << "st.y);\n}\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES:
        {
            // barycentric interpolation within a triangle.
            str << "  return ("
                << "HdGet_" << name << "_Coarse(0) * (1-st.x-st.y)"
                << " + HdGet_" << name << "_Coarse(1) * st.x"
                << " + HdGet_" << name << "_Coarse(2) * st.y);\n}\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_REFINED_QUADS:
        {
            // linear interpolation between 4 refined primvars
            str << "  ivec4 indices = HdGet_fvarIndices" << fvarChannel 
                << "();\n"
                << "  return mix("
                << "mix(" << "HdGet_" << name << "_Coarse(indices[0]),"
                <<           "HdGet_" << name << "_Coarse(indices[1]), st.s),"
                << "mix(" << "HdGet_" << name << "_Coarse(indices[3]),"
                <<           "HdGet_" << name << "_Coarse(indices[2]), st.s), "
                << "st.t);\n}\n"; 
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_REFINED_TRIANGLES:
        {
            // barycentric interpolation between 3 refined primvars
            str << "  ivec3 indices = HdGet_fvarIndices" << fvarChannel 
                << "();\n"
                << "  return ("
                << "HdGet_" << name << "_Coarse(indices[0]) * (1-st.s-st.t)"
                << " + HdGet_" << name << "_Coarse(indices[1]) * st.s"
                << " + HdGet_" << name << "_Coarse(indices[2]) * st.t);\n}\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE:
        case HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE:
        {
            // evaluation of a bspline/box spline patch
            str << "  ivec2 fvarPatchParam = HdGet_fvarPatchParam" 
                << fvarChannel << "();\n"
                << "  OsdPatchParam param = OsdPatchParamInit(fvarPatchParam.x,"
                << " fvarPatchParam.y, 0);\n"
                << "  float wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], "
                << "wDvv[20];\n"
                << "  OsdEvaluatePatchBasisNormalized(patchType, param,"
                << " st.s, st.t, wP, wDu, wDv, wDuu, wDuv, wDvv);\n"
                << "  " << _GetUnpackedType(type, false) << " result = " 
                << _GetUnpackedType(type, false) << "(0);\n"
                << "  for (int i = 0; i < HD_NUM_PATCH_VERTS; ++i) {\n"
                << "    int fvarIndex = HdGet_fvarIndices" << fvarChannel 
                << "(i);\n"
                << "    " << _GetUnpackedType(type, false) << " cv = "
                << _GetUnpackedType(type, false) << "(HdGet_" << name 
                << "_Coarse(fvarIndex));\n"
                << "    result += wP[i] * cv;\n"
                << "  }\n" 
                << " return result;\n}\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_NONE:
        {
            str << "  return HdGet_" << name << "_Coarse(localIndex);\n}\n";
            break;
        }
        default:
        {
            // emit a default version for compilation sake
            str << "  return HdGet_" << name << "_Coarse(localIndex);\n}\n";

            TF_CODING_ERROR("Face varing bindings for unexpected for" 
                            " HdSt_GeometricShader::PrimitiveType %d", 
                            (int)primType);
        }
    }

    str << "vec4 GetPatchCoord(int index);\n"
        << "vec2 GetPatchCoordLocalST();\n"
        << _GetUnpackedType(type, false)
        << " HdGet_" << name << "(int localIndex) {\n";         

    switch (fvarPatchType) {
        case HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_QUADS:         
        case HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES:
        {
            str << "  vec2 localST = GetPatchCoord(localIndex).xy;\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE:
        {
            // Compute localST in normalized patch param space
            str << "  ivec2 fvarPatchParam = HdGet_fvarPatchParam" 
                << fvarChannel << "();\n"
                << "  OsdPatchParam param = OsdPatchParamInit(fvarPatchParam.x,"
                << " fvarPatchParam.y, 0);\n"
                << "  vec2 unnormalized = GetPatchCoord(localIndex).xy;\n"
                << "  float uv[2] = float[2](unnormalized.x, unnormalized.y);\n"
                << "  OsdPatchParamNormalize(param, uv);\n"
                << "  vec2 localST = vec2(uv[0], uv[1]);\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE:
        {
            // Compute localST in normalized patch param space
            str << "  ivec2 fvarPatchParam = HdGet_fvarPatchParam" 
                << fvarChannel << "();\n"
                << "  OsdPatchParam param = OsdPatchParamInit(fvarPatchParam.x,"
                << " fvarPatchParam.y, 0);\n"
                << "  vec2 unnormalized = GetPatchCoord(localIndex).xy;\n"
                << "  float uv[2] = float[2](unnormalized.x, unnormalized.y);\n"
                << "  OsdPatchParamNormalizeTriangle(param, uv);\n"
                << "  vec2 localST = vec2(uv[0], uv[1]);\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_REFINED_QUADS:
        {
            if (hasGS) {
                str << "  vec2 lut[4] = vec2[4](vec2(0,0), vec2(1,0), "
                    << "vec2(1,1), vec2(0,1));\n"
                    << "  vec2 localST = lut[localIndex];\n";
            } else {
                str << "  vec2 localST = GetPatchCoordLocalST();\n";
            }
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_REFINED_TRIANGLES:
        {
            if (hasGS) {
                str << "  vec2 lut[3] = vec2[3](vec2(0,0), vec2(1,0), "
                    << "vec2(0,1));\n"
                    << "  vec2 localST = lut[localIndex];\n";
            } else {
                str << "  vec2 localST = GetPatchCoordLocalST();\n";
            }
            break;
        }
        default:
        {
            str << "  vec2 localST = vec2(0);\n";
        }
    }
    str << "  return HdGet_" << name << "(localIndex, localST);\n}\n";   

    // XXX: We shouldn't emit the default (argument free) accessor version,
    // since that doesn't make sense within a GS. Once we fix the XXX in
    // _GenerateShaderParameters, we should remove this.
    str << _GetUnpackedType(type, false) << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";
}

void
HdSt_CodeGen::_GenerateDrawingCoord()
{
    TF_VERIFY(_metaData.drawingCoord0Binding.binding.IsValid());
    TF_VERIFY(_metaData.drawingCoord1Binding.binding.IsValid());
    TF_VERIFY(_metaData.drawingCoord2Binding.binding.IsValid());

    /*
       hd_drawingCoord is a struct of integer offsets to locate the primvars
       in buffer arrays at the current rendering location.

       struct hd_drawingCoord {
           int modelCoord;             // (reserved) model parameters
           int constantCoord;          // constant primvars (per object)
           int vertexCoord;            // vertex primvars   (per vertex)
           int elementCoord;           // element primvars  (per face/curve)
           int primitiveCoord;         // primitive ids     (per tri/quad/line)
           int fvarCoord;              // fvar primvars     (per face-vertex)
           int shaderCoord;            // shader parameters (per shader/object)
           int topologyVisibilityCoord // topological visibility data (per face/point)
           int varyingCoord;           // varying primvars  (per vertex)
           int instanceIndex[];        // (see below)
           int instanceCoords[];       // (see below)
       };

          instanceIndex[0]  : global instance ID (used for ID rendering)
                       [1]  : instance index for level = 0
                       [2]  : instance index for level = 1
                       ...
          instanceCoords[0] : instanceDC for level = 0
          instanceCoords[1] : instanceDC for level = 1
                       ...

       We also have a drawingcoord for vertex primvars. Currently it's not
       being passed into shader since the vertex shader takes pre-offsetted
       vertex arrays and no needs to apply offset in shader (except gregory
       patch drawing etc. In that case gl_BaseVertexARB can be used under
       GL_ARB_shader_draw_parameters extention)

       gl_InstanceID is available only in vertex shader, so codegen
       takes care of applying an offset for each instance for the later
       stage. On the other hand, gl_PrimitiveID is available in all stages
       except vertex shader, and since tess/geometry shaders may or may not
       exist, we don't apply an offset of primitiveID during interstage
       plumbing to avoid overlap. Instead, GetDrawingCoord() applies
       primitiveID if necessary.

       XXX:
       Ideally we should use an interface block like:

         in DrawingCoord {
             flat hd_drawingCoord drawingCoord;
         } inDrawingCoord;
         out DrawingCoord {
             flat hd_drawingCoord drawingCoord;
         } outDrawingCoord;

      then the fragment shader can take the same input regardless the
      existence of tess/geometry shaders. However it seems the current
      driver (331.79) doesn't handle multiple interface blocks
      appropriately, it fails matching and ends up undefined results at
      consuming shader.

      > OpenGL 4.4 Core profile
      > 7.4.1 Shader Interface Matching
      >
      > When multiple shader stages are active, the outputs of one stage form
      > an interface with the inputs of the next stage. At each such
      > interface, shader inputs are matched up against outputs from the
      > previous stage:
      >
      > An output block is considered to match an input block in the
      > subsequent shader if the two blocks have the same block name, and
      > the members of the block match exactly in name, type, qualification,
      > and declaration order.
      >
      > An output variable is considered to match an input variable in the
      > subsequent shader if:
      >  - the two variables match in name, type, and qualification; or
      >  - the two variables are declared with the same location and
      >     component layout qualifiers and match in type and qualification.

      We use non-block variable for drawingCoord as a workaround of this
      problem for now. There is a caveat we can't use the same name for input
      and output, the subsequent shader has to be aware which stage writes
      the drawingCoord.

      for example:
        drawingCoord--(VS)--vsDrawingCoord--(GS)--gsDrawingCoord--(FS)
        drawingCoord--(VS)------------------------vsDrawingCoord--(FS)

      Fortunately the compiler is smart enough to optimize out unused
      attributes. If the VS writes the same value into two attributes:

        drawingCoord--(VS)--vsDrawingCoord--(GS)--gsDrawingCoord--(FS)
                      (VS)--gsDrawingCoord--------gsDrawingCoord--(FS)

      The fragment shader can always take gsDrawingCoord. The following code
      does such a plumbing work.

     */

    // common
    //
    // note: instanceCoords should be [HD_INSTANCER_NUM_LEVELS], but since
    //       GLSL doesn't allow [0] declaration, we use +1 value (WIDTH)
    //       for the sake of simplicity.
    _genCommon << "struct hd_drawingCoord {                       \n"
               << "  int modelCoord;                              \n"
               << "  int constantCoord;                           \n"
               << "  int vertexCoord;                             \n"
               << "  int elementCoord;                            \n"
               << "  int primitiveCoord;                          \n"
               << "  int fvarCoord;                               \n"
               << "  int shaderCoord;                             \n"
               << "  int topologyVisibilityCoord;                 \n"
               << "  int varyingCoord;                            \n"
               << "  int instanceIndex[HD_INSTANCE_INDEX_WIDTH];  \n"
               << "  int instanceCoords[HD_INSTANCE_INDEX_WIDTH]; \n"
               << "};\n";

    _genCommon << "hd_drawingCoord GetDrawingCoord();\n"; // forward declaration

    // vertex shader

    // [immediate]
    //   layout (location=x) uniform ivec4 drawingCoord0;
    //   layout (location=y) uniform ivec4 drawingCoord1;
    //   layout (location=z) uniform int   drawingCoordI[N];
    // [indirect]
    //   layout (location=x) in ivec4 drawingCoord0
    //   layout (location=y) in ivec4 drawingCoord1
    //   layout (location=z) in ivec2 drawingCoord2
    //   layout (location=w) in int   drawingCoordI[N]
    _EmitDeclaration(_genVS, _metaData.drawingCoord0Binding);
    _EmitDeclaration(_genVS, _metaData.drawingCoord1Binding);
    _EmitDeclaration(_genVS, _metaData.drawingCoord2Binding);
    if (_metaData.drawingCoordIBinding.binding.IsValid()) {
        _EmitDeclaration(_genVS, _metaData.drawingCoordIBinding,
                         /*arraySize=*/std::max(1, _metaData.instancerNumLevels));
    }

    std::stringstream primitiveID;

    if (HdSt_GeometricShader::IsPrimTypeTriQuads(
                                _geometricShader->GetPrimitiveType())) {
        primitiveID << "int GetPrimitiveID() {\n"
                    << "  return gl_PrimitiveID / 2;\n"
                    << "}\n"
                    << "int GetTriQuadID() {\n"
                    << "  return gl_PrimitiveID & 1;\n"
                    << "}\n";
    } else {
        primitiveID << "int GetPrimitiveID() {\n"
                    << "  return gl_PrimitiveID;\n"
                    << "}\n";
    }

    _genGS << primitiveID.str();
    _genFS << primitiveID.str();

    // instance index indirection
    _genCommon << "struct hd_instanceIndex { int indices[HD_INSTANCE_INDEX_WIDTH]; };\n";

    if (_metaData.instanceIndexArrayBinding.binding.IsValid()) {
        // << layout (location=x) uniform (int|ivec[234]) *instanceIndices;
        _EmitDeclaration(_genCommon, _metaData.instanceIndexArrayBinding);

        // << layout (location=x) uniform (int|ivec[234]) *culledInstanceIndices;
        _EmitDeclaration(_genCommon,  _metaData.culledInstanceIndexArrayBinding);

        /// if \p cullingPass is true, CodeGen generates GetInstanceIndex()
        /// such that it refers instanceIndices buffer (before culling).
        /// Otherwise, GetInstanceIndex() looks up culledInstanceIndices.

        _genVS << "int GetInstanceIndexCoord() {\n"
               << "  return drawingCoord1.y + gl_InstanceID * HD_INSTANCE_INDEX_WIDTH; \n"
               << "}\n";

        if (_geometricShader->IsFrustumCullingPass()) {
            // for frustum culling:  use instanceIndices.
            _genVS << "hd_instanceIndex GetInstanceIndex() {\n"
                   << "  int offset = GetInstanceIndexCoord();\n"
                   << "  hd_instanceIndex r;\n"
                   << "  for (int i = 0; i < HD_INSTANCE_INDEX_WIDTH; ++i)\n"
                   << "    r.indices[i] = instanceIndices[offset+i];\n"
                   << "  return r;\n"
                   << "}\n";
            _genVS << "void SetCulledInstanceIndex(uint instanceID) {\n"
                   << "  for (int i = 0; i < HD_INSTANCE_INDEX_WIDTH; ++i)\n"
                   << "    culledInstanceIndices[drawingCoord1.y + instanceID*HD_INSTANCE_INDEX_WIDTH+i]"
                   << "        = instanceIndices[drawingCoord1.y + gl_InstanceID*HD_INSTANCE_INDEX_WIDTH+i];\n"
                   << "}\n";
        } else {
            // for drawing:  use culledInstanceIndices.
            _EmitAccessor(_genVS, _metaData.culledInstanceIndexArrayBinding.name,
                          _metaData.culledInstanceIndexArrayBinding.dataType,
                          _metaData.culledInstanceIndexArrayBinding.binding,
                          "GetInstanceIndexCoord()+localIndex");
            _genVS << "hd_instanceIndex GetInstanceIndex() {\n"
                   << "  int offset = GetInstanceIndexCoord();\n"
                   << "  hd_instanceIndex r;\n"
                   << "  for (int i = 0; i < HD_INSTANCE_INDEX_WIDTH; ++i)\n"
                   << "    r.indices[i] = HdGet_culledInstanceIndices(/*localIndex=*/i);\n"
                   << "  return r;\n"
                   << "}\n";
        }
    } else {
        _genVS << "hd_instanceIndex GetInstanceIndex() {"
               << "  hd_instanceIndex r; r.indices[0] = 0; return r; }\n";
        if (_geometricShader->IsFrustumCullingPass()) {
            _genVS << "void SetCulledInstanceIndex(uint instance) "
                      "{ /*no-op*/ }\n";
        }
    }

    _genVS << "flat out hd_drawingCoord vsDrawingCoord;\n"
           // XXX: see the comment above why we need both vs and gs outputs.
           << "flat out hd_drawingCoord gsDrawingCoord;\n";

    _genVS << "hd_drawingCoord GetDrawingCoord() { hd_drawingCoord dc; \n"
           << "  dc.modelCoord              = drawingCoord0.x; \n"
           << "  dc.constantCoord           = drawingCoord0.y; \n"
           << "  dc.elementCoord            = drawingCoord0.z; \n"
           << "  dc.primitiveCoord          = drawingCoord0.w; \n"
           << "  dc.fvarCoord               = drawingCoord1.x; \n"
           << "  dc.shaderCoord             = drawingCoord1.z; \n"
           << "  dc.vertexCoord             = drawingCoord1.w; \n"
           << "  dc.topologyVisibilityCoord = drawingCoord2.x; \n"
           << "  dc.varyingCoord            = drawingCoord2.y; \n"
           << "  dc.instanceIndex           = GetInstanceIndex().indices;\n";

    if (_metaData.drawingCoordIBinding.binding.IsValid()) {
        _genVS << "  for (int i = 0; i < HD_INSTANCER_NUM_LEVELS; ++i) {\n"
               << "    dc.instanceCoords[i] = drawingCoordI[i] \n"
               << "      + dc.instanceIndex[i+1]; \n"
               << "  }\n";
    }

    _genVS << "  return dc;\n"
           << "}\n";

    // note: GL spec says tessellation input array size must be equal to
    //       gl_MaxPatchVertices, which is used for intrinsic declaration
    //       of built-in variables:
    //       in gl_PerVertex {} gl_in[gl_MaxPatchVertices];

    // tess control shader
    _genTCS << "flat in hd_drawingCoord vsDrawingCoord[gl_MaxPatchVertices];\n"
            << "flat out hd_drawingCoord tcsDrawingCoord[HD_NUM_PATCH_EVAL_VERTS];\n"
            << "hd_drawingCoord GetDrawingCoord() { \n"
            << "  hd_drawingCoord dc = vsDrawingCoord[0];\n"
            << "  dc.primitiveCoord += gl_PrimitiveID;\n"
            << "  return dc;\n"
            << "}\n";
    // tess eval shader
    _genTES << "flat in hd_drawingCoord tcsDrawingCoord[gl_MaxPatchVertices];\n"
            << "flat out hd_drawingCoord vsDrawingCoord;\n"
            << "flat out hd_drawingCoord gsDrawingCoord;\n"
            << "hd_drawingCoord GetDrawingCoord() { \n"
            << "  hd_drawingCoord dc = tcsDrawingCoord[0]; \n"
            << "  dc.primitiveCoord += gl_PrimitiveID; \n"
            << "  return dc;\n"
            << "}\n";

    // geometry shader ( VSdc + gl_PrimitiveIDIn )
    _genGS << "flat in hd_drawingCoord vsDrawingCoord[HD_NUM_PRIMITIVE_VERTS];\n"
           << "flat out hd_drawingCoord gsDrawingCoord;\n"
           << "hd_drawingCoord GetDrawingCoord() { \n"
           << "  hd_drawingCoord dc = vsDrawingCoord[0]; \n"
           << "  dc.primitiveCoord += GetPrimitiveID(); \n"
           << "  return dc; \n"
           << "}\n";

    // fragment shader ( VSdc + gl_PrimitiveID )
    // note that gsDrawingCoord isn't offsetted by gl_PrimitiveIDIn
    _genFS << "flat in hd_drawingCoord gsDrawingCoord;\n"
           << "hd_drawingCoord GetDrawingCoord() { \n"
           << "  hd_drawingCoord dc = gsDrawingCoord; \n"
           << "  dc.primitiveCoord += GetPrimitiveID(); \n"
           << "  return dc; \n"
           << "}\n";

    // drawing coord plumbing.
    // Note that copying from [0] for multiple input source since the
    // drawingCoord is flat (no interpolation required).
    _procVS  << "  vsDrawingCoord = GetDrawingCoord();\n"
             << "  gsDrawingCoord = GetDrawingCoord();\n";
    _procTCS << "  tcsDrawingCoord[gl_InvocationID] = "
             << "  vsDrawingCoord[gl_InvocationID];\n";
    _procTES << "  vsDrawingCoord = tcsDrawingCoord[0];\n"
             << "  gsDrawingCoord = tcsDrawingCoord[0];\n";
    _procGS  << "  gsDrawingCoord = vsDrawingCoord[0];\n";

}
void
HdSt_CodeGen::_GenerateConstantPrimvar()
{
    /*
      // --------- constant data declaration ----------
      struct ConstantData0 {
          mat4 transform;
          mat4 transformInverse;
          mat4 instancerTransform[2];
          vec3 displayColor;
          vec4 primID;
      };
      // bindless
      layout (location=0) uniform ConstantData0 *constantData0;
      // not bindless
      layout (std430, binding=0) buffer {
          constantData0 constantData0[];
      };

      // --------- constant data accessors ----------
      mat4 HdGet_transform(int localIndex) {
          return constantData0[GetConstantCoord()].transform;
      }
      vec3 HdGet_displayColor(int localIndex) {
          return constantData0[GetConstantCoord()].displayColor;
      }

    */

    std::stringstream declarations;
    std::stringstream accessors;
    TF_FOR_ALL (it, _metaData.constantData) {
        // note: _constantData has been sorted by offset in HdSt_ResourceBinder.
        // XXX: not robust enough, should consider padding and layouting rules
        // to match with the logic in HdInterleavedMemoryManager if we
        // want to use a layouting policy other than default padding.

        HdBinding binding = it->first;
        TfToken typeName(TfStringPrintf("ConstantData%d", binding.GetValue()));
        TfToken varName = it->second.blockName;

        declarations << "struct " << typeName << " {\n";

        TF_FOR_ALL (dbIt, it->second.entries) {
            if (!TF_VERIFY(!dbIt->dataType.IsEmpty(),
                              "Unknown dataType for %s",
                              dbIt->name.GetText())) {
                continue;
            }

            declarations << "  " << _GetPackedType(dbIt->dataType, false)
                         << " " << dbIt->name;
            if (dbIt->arraySize > 1) {
                declarations << "[" << dbIt->arraySize << "]";
            }

            declarations << ";\n";

            _EmitStructAccessor(accessors, varName, dbIt->name, dbIt->dataType,
                                dbIt->arraySize,
                                "GetDrawingCoord().constantCoord");
        }
        declarations << "};\n";

        // XXX: passing arraySize=2 to cheat driver to not tell actual size.
        //      we should compute the actual size or maximum size if possible.
        _EmitDeclaration(declarations, varName, typeName, binding, /*arraySize=*/1);
    }
    _genCommon << declarations.str()
               << accessors.str();
}

void
HdSt_CodeGen::_GenerateInstancePrimvar()
{
    /*
      // --------- instance data declaration ----------
      // bindless
      layout (location=X) uniform vec4 *data;
      // not bindless
      layout (std430, binding=X) buffer buffer_X {
          vec4 data[];
      };

      // --------- instance data accessors ----------
      vec3 HdGet_translate(int localIndex=0) {
          return instanceData0[GetInstanceCoord()].translate;
      }
    */

    std::stringstream declarations;
    std::stringstream accessors;

    struct LevelEntries {
        TfToken dataType;
        std::vector<int> levels;
    };
    std::map<TfToken, LevelEntries> nameAndLevels;

    TF_FOR_ALL (it, _metaData.instanceData) {
        HdBinding binding = it->first;
        TfToken const &dataType = it->second.dataType;
        int level = it->second.level;

        nameAndLevels[it->second.name].dataType = dataType;
        nameAndLevels[it->second.name].levels.push_back(level);

        std::stringstream n;
        n << it->second.name << "_" << level;
        TfToken name(n.str());
        n.str("");
        n << "GetDrawingCoord().instanceCoords[" << level << "]";

        // << layout (location=x) uniform float *translate_0;
        _EmitDeclaration(declarations, name, dataType, binding);
        _EmitAccessor(accessors, name, dataType, binding, n.str().c_str());

    }

    /*
      accessor taking level as a parameter.
      note that instance primvar may or may not be defined for each level.
      we expect level is an unrollable constant to optimize out branching.

      vec3 HdGetInstance_translate(int level, vec3 defaultValue) {
          if (level == 0) return HdGet_translate_0();
          // level==1 is not defined. use default
          if (level == 2) return HdGet_translate_2();
          if (level == 3) return HdGet_translate_3();
          return defaultValue;
      }
    */
    TF_FOR_ALL (it, nameAndLevels) {
        accessors << _GetUnpackedType(it->second.dataType, false)
                  << " HdGetInstance_" << it->first << "(int level, "
                  << _GetUnpackedType(it->second.dataType, false)
                  << " defaultValue) {\n";
        TF_FOR_ALL (levelIt, it->second.levels) {
            accessors << "  if (level == " << *levelIt << ") "
                      << "return HdGet_" << it->first << "_" << *levelIt << "();\n";
        }

        accessors << "  return defaultValue;\n"
                  << "}\n";
    }
    /*
      common accessor, if the primvar is defined on the instancer but not
      the rprim.

      #if !defined(HD_HAS_translate)
      #define HD_HAS_translate 1
      vec3 HdGet_translate(int localIndex) {
          // 0 is the lowest level for which this is defined
          return HdGet_translate_0();
      }
      vec3 HdGet_translate() {
          return HdGet_translate(0);
      }
      #endif
    */
    TF_FOR_ALL (it, nameAndLevels) {
        accessors << "#if !defined(HD_HAS_" << it->first << ")\n"
                  << "#define HD_HAS_" << it->first << " 1\n"
                  << _GetUnpackedType(it->second.dataType, false)
                  << " HdGet_" << it->first << "(int localIndex) {\n"
                  << "  return HdGet_" << it->first << "_"
                                       << it->second.levels.front() << "();\n"
                  << "}\n"
                  << _GetUnpackedType(it->second.dataType, false)
                  << " HdGet_" << it->first << "() { return HdGet_"
                  << it->first << "(0); }\n"
                  << "#endif\n";
    }
        

    _genCommon << declarations.str()
               << accessors.str();
}

void
HdSt_CodeGen::_GenerateElementPrimvar()
{
    /*
    Accessing uniform primvar data:
    ===============================
    Uniform primvar data is authored at the subprimitive (also called element or
    face below) granularity.
    To access uniform primvar data (say color), there are two indirections in
    the lookup because of aggregation in the buffer layout.
          ----------------------------------------------------
    color | prim0 colors | prim1 colors | .... | primN colors|
          ----------------------------------------------------
    For each prim, GetDrawingCoord().elementCoord holds the start index into
    this buffer.

    For an unrefined prim, the subprimitive ID is simply the gl_PrimitiveID.
    For a refined prim, gl_PrimitiveID corresponds to the refined element ID.

    To map a refined face to its coarse face, Storm builds a "primitive param"
    buffer (more details in the section below). This buffer is also aggregated,
    and for each subprimitive, GetDrawingCoord().primitiveCoord gives us the
    index into this buffer (meaning it has already added the gl_PrimitiveID)

    To have a single codepath for both cases, we build the primitive param
    buffer for unrefined prims as well, and effectively index the uniform
    primvar using:
    drawCoord.elementCoord + primitiveParam[ drawCoord.primitiveCoord ]

    The code generated looks something like:

      // --------- primitive param declaration ----------
      struct PrimitiveData { int elementID; }
      layout (std430, binding=?) buffer PrimitiveBuffer {
          PrimitiveData primitiveData[];
      };

      // --------- indirection accessors ---------
      // Gives us the "coarse" element ID
      int GetElementID() {
          return primitiveData[GetPrimitiveCoord()].elementID;
      }
      
      // Adds the offset to the start of the uniform primvar data for the prim
      int GetAggregatedElementID() {
          return GetElementID() + GetDrawingCoord().elementCoord;\n"
      }

      // --------- uniform primvar declaration ---------
      struct ElementData0 {
          vec3 displayColor;
      };
      layout (std430, binding=?) buffer buffer0 {
          ElementData0 elementData0[];
      };

      // ---------uniform primvar data accessor ---------
      vec3 HdGet_displayColor(int localIndex) {
          return elementData0[GetAggregatedElementID()].displayColor;
      }

    */

    // Primitive Param buffer layout:
    // ==============================
    // Depending on the prim, one of following is used:
    // 
    // 1. basis curves
    //     1 int  : curve index 
    //     
    //     This lets us translate a basis curve segment to its curve id.
    //     A basis curve is made up for 'n' curves, each of which have a varying
    //     number of segments.
    //     (see hdSt/basisCurvesComputations.cpp)
    //     
    // 2. mesh specific
    // a. tris
    //     1 int  : coarse face index + edge flag
    //     (see hd/meshUtil.h,cpp)
    //     
    // b. quads coarse
    //     2 ints : coarse face index + edge flag
    //              ptex index
    //     (see hd/meshUtil.h,cpp)
    //
    // c. tris & quads uniformly refined
    //     3 ints : coarse face index + edge flag
    //              Far::PatchParam::field0 (includes ptex index)
    //              Far::PatchParam::field1
    //     (see hdSt/subdivision.cpp)
    //
    // d. patch adaptively refined
    //     4 ints : coarse face index + edge flag
    //              Far::PatchParam::field0 (includes ptex index)
    //              Far::PatchParam::field1
    //              sharpness (float)
    //     (see hdSt/subdivision.cpp)
    // -----------------------------------------------------------------------
    // note: decoding logic of primitiveParam has to match with
    // HdMeshTopology::DecodeFaceIndexFromPrimitiveParam()
    //
    // PatchParam is defined as ivec3 (see opensubdiv/far/patchParam.h)
    //  Field0     | Bits | Content
    //  -----------|:----:|---------------------------------------------------
    //  faceId     | 28   | the faceId of the patch (Storm uses ptexIndex)
    //  transition | 4    | transition edge mask encoding
    //
    //  Field1     | Bits | Content
    //  -----------|:----:|---------------------------------------------------
    //  level      | 4    | the subdivision level of the patch
    //  nonquad    | 1    | whether patch is refined from a non-quad face
    //  regular    | 1    | whether patch is regular
    //  unused     | 1    | unused
    //  boundary   | 5    | boundary mask encoding
    //  v          | 10   | log2 value of u parameter at first patch corner
    //  u          | 10   | log2 value of v parameter at first patch corner
    //
    //  Field2     (float)  sharpness
    //
    // whereas adaptive patches have PatchParams computed by OpenSubdiv,
    // we need to construct PatchParams for coarse tris and quads.
    // Currently it's enough to fill just faceId for coarse quads for
    // ptex shading.

    std::stringstream declarations;
    std::stringstream accessors;

    if (_metaData.primitiveParamBinding.binding.IsValid()) {

        HdBinding binding = _metaData.primitiveParamBinding.binding;
        _EmitDeclaration(declarations, _metaData.primitiveParamBinding);
        _EmitAccessor(accessors, _metaData.primitiveParamBinding.name,
                        _metaData.primitiveParamBinding.dataType, binding,
                        "GetDrawingCoord().primitiveCoord");

        if (_geometricShader->IsPrimTypePoints()) {
            // do nothing. 
            // e.g. if a prim's geomstyle is points and it has a valid
            // primitiveParamBinding, we don't generate any of the 
            // accessor methods.
            ;            
        }
        else if (_geometricShader->IsPrimTypeBasisCurves()) {
            // straight-forward indexing to get the segment's curve id
            accessors
                << "int GetElementID() {\n"
                << "  return (hd_int_get(HdGet_primitiveParam()));\n"
                << "}\n";
            accessors
                << "int GetAggregatedElementID() {\n"
                << "  return GetElementID()\n"
                << "  + GetDrawingCoord().elementCoord;\n"
                << "}\n";
        }
        else if (_geometricShader->IsPrimTypeMesh()) {
            // GetPatchParam, GetEdgeFlag
            switch (_geometricShader->GetPrimitiveType()) {
                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_QUADS:
                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIQUADS:
                {
                    // refined quads (catmulClark uniform subdiv) or
                    // refined tris (loop uniform subdiv)
                    accessors
                        << "ivec3 GetPatchParam() {\n"
                        << "  return ivec3(HdGet_primitiveParam().y, \n"
                        << "               HdGet_primitiveParam().z, 0);\n"
                        << "}\n";
                    accessors
                        << "int GetEdgeFlag() {\n"
                        << "  return (HdGet_primitiveParam().x & 3);\n"
                        << "}\n";
                    break;
                }

                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BSPLINE:
                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
                {
                    // "adaptive" subdivision generates refined patches 
                    // (tessellated triangles)
                    accessors
                        << "ivec3 GetPatchParam() {\n"
                        << "  return ivec3(HdGet_primitiveParam().y, \n"
                        << "               HdGet_primitiveParam().z, \n"
                        << "               HdGet_primitiveParam().w);\n"
                        << "}\n";
                    accessors
                        << "int GetEdgeFlag() {\n"
                        << "  return (HdGet_primitiveParam().x & 3);\n"
                        << "}\n";
                    break;
                }

                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_QUADS:
                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
                case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIQUADS:
                {
                    // coarse quads or coarse triangles
                    // ptexId matches the primitiveID for quadrangulated or
                    // triangulated meshes, the other fields can be left as 0.
                    // When there are geom subsets, we can no longer use the 
                    // primitiveId and instead use a buffer source generated
                    // per subset draw item containing the coarse face indices. 
                    accessors
                        << "#if defined(HD_HAS_coarseFaceIndex)\n"
                        << "int HdGet_coarseFaceIndex();\n"
                        << "#endif\n"
                        << "ivec3 GetPatchParam() {\n"
                        << "#if defined(HD_HAS_coarseFaceIndex)\n "
                        << "  return ivec3(HdGet_coarseFaceIndex().x, 0, 0);\n"
                        << "#else\n "
                        << "  return ivec3(GetPrimitiveID(), 0, 0);\n"
                        << "#endif\n"
                        << "}\n";
                    // edge flag encodes edges which have been
                    // introduced by quadrangulation or triangulation
                    accessors
                        << "int GetEdgeFlag() {\n"
                        << "  return (HdGet_primitiveParam() & 3);\n"
                        << "}\n";
                    break;
                }

                default:
                {
                    TF_CODING_ERROR("HdSt_GeometricShader::PrimitiveType %d is "
                      "unexpected in _GenerateElementPrimvar().",
                      (int)_geometricShader->GetPrimitiveType());
                }
            }

            // GetFVarIndex
            if (_geometricShader->GetFvarPatchType() == 
                HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_TRIANGLES) 
            {
                // note that triangulated meshes don't have ptexIndex.
                // Here we're passing primitiveID as ptexIndex PatchParam
                // since Hd_TriangulateFaceVaryingComputation unrolls facevaring
                // primvars for each triangles.
                accessors
                    << "int GetFVarIndex(int localIndex) {\n"
                    << "  int fvarCoord = GetDrawingCoord().fvarCoord;\n"
                    << "  int ptexIndex = GetPatchParam().x & 0xfffffff;\n"
                    << "  return fvarCoord + ptexIndex * 3 + localIndex;\n"
                    << "}\n";    
            } else if (_geometricShader->GetFvarPatchType() == 
                HdSt_GeometricShader::FvarPatchType::PATCH_COARSE_QUADS) {
                accessors
                    << "int GetFVarIndex(int localIndex) {\n"
                    << "  int fvarCoord = GetDrawingCoord().fvarCoord;\n"
                    << "  int ptexIndex = GetPatchParam().x & 0xfffffff;\n"
                    << "  return fvarCoord + ptexIndex * 4 + localIndex;\n"
                    << "}\n";
            }

            // ElementID getters
            accessors
                << "int GetElementID() {\n"
                << "  return (hd_int_get(HdGet_primitiveParam()) >> 2);\n"
                << "}\n";

            accessors
                << "int GetAggregatedElementID() {\n"
                << "  return GetElementID()\n"
                << "  + GetDrawingCoord().elementCoord;\n"
                << "}\n";
        }
        else {
            TF_CODING_ERROR("HdSt_GeometricShader::PrimitiveType %d is "
                  "unexpected in _GenerateElementPrimvar().",
                  (int)_geometricShader->GetPrimitiveType());
        }
    } else {
        // no primitiveParamBinding

        // XXX: this is here only to keep the compiler happy, we don't expect
        // users to call them -- we really should restructure whatever is
        // necessary to avoid having to do this and thus guarantee that users
        // can never call bogus versions of these functions.

        // Use a fallback of -1, so that points aren't selection highlighted
        // when face 0 is selected. This would be the case if we returned 0,
        // since the selection highlighting code is repr-agnostic.
        // It is safe to do this for points, since  we don't generate accessors 
        // for element primvars, and thus don't use it as an index into
        // elementCoord.
        if (_geometricShader->IsPrimTypePoints()) {
            accessors
              << "int GetElementID() {\n"
              << "  return -1;\n"
              << "}\n";  
        } else {
            accessors
                << "int GetElementID() {\n"
                << "  return 0;\n"
                << "}\n";
        }
        accessors
            << "int GetAggregatedElementID() {\n"
            << "  return GetElementID();\n"
            << "}\n";
        accessors
            << "int GetEdgeFlag() {\n"
            << "  return 0;\n"
            << "}\n";
        accessors
            << "ivec3 GetPatchParam() {\n"
            << "  return ivec3(0, 0, 0);\n"
            << "}\n";
        accessors
            << "int GetFVarIndex(int localIndex) {\n"
            << "  return 0;\n"
            << "}\n";
    }
    declarations
        << "int GetElementID();\n"
        << "int GetAggregatedElementID();\n";


    if (_metaData.edgeIndexBinding.binding.IsValid()) {

        HdBinding binding = _metaData.edgeIndexBinding.binding;

        _EmitDeclaration(declarations, _metaData.edgeIndexBinding);
        _EmitAccessor(accessors, _metaData.edgeIndexBinding.name,
                    _metaData.edgeIndexBinding.dataType, binding,
                    "GetDrawingCoord().primitiveCoord");
    }

    if (_metaData.coarseFaceIndexBinding.binding.IsValid()) {
        _genCommon << "#define HD_HAS_" 
            << _metaData.coarseFaceIndexBinding.name << " 1\n";

        const HdBinding &binding = _metaData.coarseFaceIndexBinding.binding;

        _EmitDeclaration(declarations, _metaData.coarseFaceIndexBinding);
        _EmitAccessor(accessors, _metaData.coarseFaceIndexBinding.name,
                    _metaData.coarseFaceIndexBinding.dataType, binding,
                    "GetDrawingCoord().primitiveCoord  + localIndex");
    }

    switch (_geometricShader->GetPrimitiveType()) {
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_QUADS:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_REFINED_TRIQUADS:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BSPLINE:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIQUADS:
            // This is no longer used by Storm but is generated for backward
            // compatibility with production shaders.
            accessors
                << "int GetAuthoredEdgeId(int primitiveEdgeId) {\n"
                << "  return primitiveEdgeId;\n"
                << "}\n";
            break;
        default:
            // The functions below are used in picking (id render) and/or
            // selection highlighting, and are expected to be defined.
            // Generate fallback versions when we aren't rendering meshes.
            accessors
                << "int GetAuthoredEdgeId(int primitiveEdgeId) {\n"
                << "  return -1;\n"
                << "}\n";
            accessors
                << "int GetPrimitiveEdgeId() {\n"
                << "  return -1;\n"
                << "}\n";
            accessors
                << "float GetSelectedEdgeOpacity() {\n"
                << "  return 0.0;\n"
                << "}\n";
            break;
    }

    declarations
        << "int GetPrimitiveEdgeId();\n"
        << "float GetSelectedEdgeOpacity();\n";

    // Uniform primvar data declarations & accessors
    if (!_geometricShader->IsPrimTypePoints()) {
        TF_FOR_ALL (it, _metaData.elementData) {
            HdBinding binding = it->first;
            TfToken const &name = it->second.name;
            TfToken const &dataType = it->second.dataType;

            _EmitDeclaration(declarations, name, dataType, binding);
            // AggregatedElementID gives us the buffer index post batching, which
            // is what we need for accessing element (uniform) primvar data.
            _EmitAccessor(accessors, name, dataType, binding,"GetAggregatedElementID()");
        }
    }

    for (size_t i = 0; i < _metaData.fvarIndicesBindings.size(); ++i) {
        if (!_metaData.fvarIndicesBindings[i].binding.IsValid()) {
            continue;
        }

        HdBinding binding = _metaData.fvarIndicesBindings[i].binding;
        TfToken name = _metaData.fvarIndicesBindings[i].name;
        _EmitDeclaration(declarations, name, 
            _metaData.fvarIndicesBindings[i].dataType, 
            _metaData.fvarIndicesBindings[i].binding, 0);

        if (_geometricShader->GetFvarPatchType() == 
            HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE || 
            _geometricShader->GetFvarPatchType() ==
            HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
            _EmitAccessor(accessors, name,
                _metaData.fvarIndicesBindings[i].dataType, binding,
                "GetDrawingCoord().primitiveCoord * HD_NUM_PATCH_VERTS + "
                "localIndex");
        } else {
            _EmitAccessor(accessors,name,
                _metaData.fvarIndicesBindings[i].dataType, binding,
                "GetDrawingCoord().primitiveCoord + localIndex");
        }
    }

    for (size_t i = 0; i < _metaData.fvarPatchParamBindings.size(); ++i) {
        if (!_metaData.fvarPatchParamBindings[i].binding.IsValid()) {
            continue;
        }

        HdBinding binding = _metaData.fvarPatchParamBindings[i].binding;
        TfToken name = _metaData.fvarPatchParamBindings[i].name;
        _EmitDeclaration(declarations, name, 
            _metaData.fvarPatchParamBindings[i].dataType, 
            _metaData.fvarPatchParamBindings[i].binding, 0);

        // Only need fvar patch param for bspline or box spline patches
        if (_geometricShader->GetFvarPatchType() == 
            HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE ||
            _geometricShader->GetFvarPatchType() ==
            HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
            _EmitAccessor(accessors, name,
                _metaData.fvarPatchParamBindings[i].dataType, binding,
                "GetDrawingCoord().primitiveCoord + localIndex");
        }
    }

    // Emit primvar declarations and accessors.
    _genTCS << declarations.str()
            << accessors.str();
    _genTES << declarations.str()
            << accessors.str();
    _genGS << declarations.str()
           << accessors.str();
    _genFS << declarations.str()
           << accessors.str();
}

void
HdSt_CodeGen::_GenerateVertexAndFaceVaryingPrimvar(bool hasGS, 
    bool shaderDrawParametersEnabled, int glslVersion)
{
    // VS specific accessor for the "vertex drawing coordinate"
    // Even though we currently always plumb vertexCoord as part of the drawing
    // coordinate, we expect clients to use this accessor when querying the base
    // vertex offset for a draw call.
    _genVS << "int GetBaseVertexOffset() {\n";
    if (shaderDrawParametersEnabled) {
        if (glslVersion < 460) { // use ARB extension
            _genVS << "  return gl_BaseVertexARB;\n";
        } else {
            _genVS << "  return gl_BaseVertex;\n";
        }
    } else {
        _genVS << "  return GetDrawingCoord().vertexCoord;\n";
    }
    _genVS << "}\n";
    
    // Vertex, Varying, and FVar primvar flow into the fragment shader as 
    // per-fragment attribute data that has been interpolated by the rasterizer,
    // and hence have similarities for code gen.
    // While vertex primvar are authored per vertex and require plumbing
    // through all shader stages, fVar is emitted only in the GS stage.
    // Varying primvar are bound in the VS via buffer array but are processed as 
    // vertex data for the rest of the stages.
    /*
      // --------- vertex data declaration (VS) ----------
      layout (location = 0) in vec3 normals;
      layout (location = 1) in vec3 points;

      out Primvars {
          vec3 normals;
          vec3 points;
      } outPrimvars;

      void ProcessPrimvars() {
          outPrimvars.normals = normals;
          outPrimvars.points = points;
      }

      // --------- geometry stage plumbing -------
      in Primvars {
          vec3 normals;
          vec3 points;
      } inPrimvars[];
      out Primvars {
          vec3 normals;
          vec3 points;
      } outPrimvars;

      void ProcessPrimvars(int index) {
          outPrimvars = inPrimvars[index];
      }

      // --------- vertex/varying data accessors (used in GS/FS) ---
      in Primvars {
          vec3 normals;
          vec3 points;
      } inPrimvars;
      vec3 HdGet_normals(int localIndex=0) {
          return inPrimvars.normals;
      }
    */

    std::stringstream vertexInputs;
    std::stringstream interstageVertexData;
    std::stringstream accessorsVS, accessorsTCS, accessorsTES,
        accessorsGS, accessorsFS;

    // vertex 
    TF_FOR_ALL (it, _metaData.vertexData) {
        HdBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;

        // future work:
        // with ARB_enhanced_layouts extention, it's possible
        // to use "component" qualifier to declare offsetted primvars
        // in interleaved buffer.
        _EmitDeclaration(vertexInputs, name, dataType, binding);

        interstageVertexData << "  " << _GetPackedType(dataType, false)
                             << " " << name << ";\n";

        // primvar accessors
        _EmitAccessor(accessorsVS, name, dataType, binding);

        _EmitStructAccessor(accessorsTCS, pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "gl_InvocationID");
        _EmitStructAccessor(accessorsTES, pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsGS,  pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsFS,  pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1);

        // interstage plumbing
        _procVS << "  outPrimvars." << name
                << " = " << name << ";\n";
        _procTCS << "  outPrimvars[gl_InvocationID]." << name
                 << " = inPrimvars[gl_InvocationID]." << name << ";\n";
        _procTES << "  outPrimvars." << name
                 << " = basis[0] * inPrimvars[i0]." << name
                 << " + basis[1] * inPrimvars[i1]." << name
                 << " + basis[2] * inPrimvars[i2]." << name
                 << " + basis[3] * inPrimvars[i3]." << name << ";\n";
        _procGS  << "  outPrimvars." << name
                 << " = inPrimvars[index]." << name << ";\n";
    }

    /*
      // --------- varying data declaration (VS) ----------------
      layout (std430, binding=?) buffer buffer0 {
          vec3 displayColor[];
      };

      vec3 HdGet_displayColor(int localIndex) {
        int index =  GetDrawingCoord().varyingCoord + gl_VertexID - 
            GetBaseVertexOffset();
        return vec3(displayColor[index]);
      }
      vec3 HdGet_displayColor() { return HdGet_displayColor(0); }

      out Primvars {
          vec3 displayColor;
      } outPrimvars;

      void ProcessPrimvars() {
          outPrimvars.displayColor = HdGet_displayColor();
      }

      // --------- fragment stage plumbing -------
      in Primvars {
          vec3 displayColor;
      } inPrimvars;
    */

    std::stringstream varyingDeclarations;

    TF_FOR_ALL (it, _metaData.varyingData) {
        HdBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;

        _EmitDeclaration(varyingDeclarations, name, dataType, binding);

        interstageVertexData << "  " << _GetPackedType(dataType, false)
                             << " " << name << ";\n";

        // primvar accessors
        _EmitBufferAccessor(accessorsVS, name, dataType, 
            "GetDrawingCoord().varyingCoord + gl_VertexID - GetBaseVertexOffset()");
        _EmitStructAccessor(accessorsTCS, pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "gl_InvocationID");
        _EmitStructAccessor(accessorsTES, pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsGS,  pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsFS,  pxrImagingHdStCodeGen::_tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1);

        // interstage plumbing
        _procVS << "  outPrimvars." << name
                << " = " << "HdGet_" << name << "();\n";
        _procTCS << "  outPrimvars[gl_InvocationID]." << name
                 << " = inPrimvars[gl_InvocationID]." << name << ";\n";
        _procTES << "  outPrimvars." << name  << " = ProcessPrimvar("
                 << "inPrimvars[i0]." << name 
                 << ", inPrimvars[i1]." << name 
                 << ", inPrimvars[i2]." << name 
                 << ", inPrimvars[i3]." << name 
                 << ", basis, uv);\n";
        _procGS  << "  outPrimvars." << name
                 << " = inPrimvars[index]." << name << ";\n";
    }

    /*
      // --------- facevarying data declaration ----------------
      layout (std430, binding=?) buffer buffer0 {
          vec2 map1[];
      };
      layout (std430, binding=?) buffer buffer1 {
          float map2_u[];
      };

      // --------- geometry stage plumbing -------
      out Primvars {
          ...
          vec2 map1;
          float map2_u;
      } outPrimvars;

      void ProcessPrimvars(int index) {
          outPrimvars.map1 = HdGet_map1(index, localST);
          outPrimvars.map2_u = HdGet_map2_u(index, localST);
      }

      // --------- fragment stage plumbing -------
      in Primvars {
          ...
          vec2 map1;
          float map2_u;
      } inPrimvars;

      // --------- facevarying data accessors ----------
      // in geometry shader
      // unrefined internal accessor
      vec2 HdGet_map1_Coarse(int localIndex) {
          int fvarIndex = GetFVarIndex(localIndex);
          return vec2(map1[fvarIndex]);
      }
      // unrefined public accessors
      vec2 HdGet_map1(int localIndex, vec2 st) {
          int fvarIndex = GetFVarIndex(localIndex);
          return (HdGet_map1_Coarse(0) * ...);
      }
      vec2 HdGet_map1(int localIndex) {
          vec2 localST = GetPatchCoord(localIndex).xy;
          return HdGet_map1(localIndex, localST);
      }

      // refined internal accessor
      vec2 HdGet_map1_Coarse(int localIndex) {
          int fvarIndex = GetDrawingCoord().fvarCoord + localIndex;
          return vec2(map1[fvarIndex]);
      }
      // refined public accessors
      vec2 HdGet_map1(int localIndex, vec2 st) {
          ivec4 indices = HdGet_fvarIndices0();
          return mix(mix(HdGet_map1_Coarse(indices[0])...);
      }
      // refined quads:
      vec2 HdGet_map1(int localIndex) {
          vec2 lut[4] = vec2[4](vec2(0,0), vec2(1,0), vec2(1,1), vec2(0,1));
          vec2 localST = lut[localIndex];\n";
          return HdGet_map1(localIndex, localST);
      }
      // refined triangles:
      vec2 HdGet_map1(int localIndex) {
          vec2 lut[3] = vec2[3](vec2(0,0), vec2(1,0), vec2(0,1));
          vec2 localST = lut[localIndex];\n";
          return HdGet_map1(localIndex, localST);
      }

      // refined public accessor for b-spline/box-spline patches
      vec2 HdGet_map1(int localIndex, vec2 st) {
          int patchType = OSD_PATCH_DESCRIPTOR_REGULAR; // b-spline patches
          // OR int patchType = OSD_PATCH_DESCRIPTOR_LOOP; for box-spline
          ivec2 fvarPatchParam = HdGet_fvarPatchParam0();
          OsdPatchParam param = OsdPatchParamInit(fvarPatchParam.x, 
                                                  fvarPatchParam.y, 0);
          float wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], wDvv[20];
          OsdEvaluatePatchBasisNormalized(patchType, param, st.s, 
            st.t, wP, wDu, wDv, wDuu, wDuv, wDvv);
          vec2 result = vec2(0);
          for (int i = 0; i < HD_NUM_PATCH_VERTS; ++i) {
              int fvarIndex = HdGet_fvarIndices0(i);
               vec2 cv = vec2(HdGet_map1_Coarse(fvarIndex));
               result += wP[i] * cv;
          }
          return result;
      }
   
      // in fragment shader
      vec2 HdGet_map1() {
          return inPrimvars.map1;
      }
    */

    // face varying
    std::stringstream fvarDeclarationsGS;
    std::stringstream fvarDeclarationsFS;
    std::stringstream interstageFVarData;

    // FVar primvars are emitted by GS or FS
    TF_FOR_ALL (it, _metaData.fvarData) {
        HdBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;
        const int channel = it->second.channel;

        if (hasGS) {
            _EmitDeclaration(fvarDeclarationsGS, name, dataType, binding);

            interstageFVarData << "  " << _GetPackedType(dataType, false)
                               << " " << name << ";\n";

            // primvar accessors (only in GS and FS)
            _EmitFVarAccessor(hasGS, accessorsGS, name, dataType, binding,
                              _geometricShader->GetPrimitiveType(),
                              _geometricShader->GetFvarPatchType(),
                              channel);
        
            _EmitStructAccessor(accessorsFS, pxrImagingHdStCodeGen::_tokens->inPrimvars,
                                name, dataType,
                                /*arraySize=*/1, NULL);

            if (_geometricShader->GetFvarPatchType() == 
                HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE ||
                _geometricShader->GetFvarPatchType() == 
                HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
                    _procGS << "  outPrimvars." << name 
                            << " = HdGet_" << name << "(index, localST);\n";
            } else {
                _procGS << "  outPrimvars." << name 
                        << " = HdGet_" << name << "(index);\n";
            }
        } else if (!_geometricShader->IsPrimTypePoints()) {
            _EmitDeclaration(fvarDeclarationsFS, name, dataType, binding);

            _EmitFVarAccessor(hasGS, accessorsFS, name, dataType, binding,
                              _geometricShader->GetPrimitiveType(),
                              _geometricShader->GetFvarPatchType(),
                              channel);
        }
    }

    _genGS  << fvarDeclarationsGS.str();
    _genFS  << fvarDeclarationsFS.str();

    if (!interstageVertexData.str().empty()) {
        _genVS  << vertexInputs.str()
                << varyingDeclarations.str()
                << "out Primvars {\n"
                << interstageVertexData.str()
                << "} outPrimvars;\n"
                << accessorsVS.str();

        _genTCS << "in Primvars {\n"
                << interstageVertexData.str()
                << "} inPrimvars[gl_MaxPatchVertices];\n"
                << "out Primvars {\n"
                << interstageVertexData.str()
                << "} outPrimvars[HD_NUM_PATCH_EVAL_VERTS];\n"
                << accessorsTCS.str();

        _genTES << "in Primvars {\n"
                << interstageVertexData.str()
                << "} inPrimvars[gl_MaxPatchVertices];\n"
                << "out Primvars {\n"
                << interstageVertexData.str()
                << "} outPrimvars;\n"
                << accessorsTES.str();

        _genGS  << "in Primvars {\n"
                << interstageVertexData.str()
                << "} inPrimvars[HD_NUM_PRIMITIVE_VERTS];\n";
    }

    if (!interstageVertexData.str().empty() ||
        !interstageFVarData.str().empty()) {
        _genGS  << "out Primvars {\n"
                << interstageVertexData.str()
                << interstageFVarData.str()
                << "} outPrimvars;\n";

        _genFS  << "in Primvars {\n"
                << interstageVertexData.str()
                << interstageFVarData.str()
                << "} inPrimvars;\n";
    }

    _genGS  << accessorsGS.str();
    _genFS  << accessorsFS.str();

    // ---------
    _genFS << "vec4 GetPatchCoord(int index);\n";
    _genFS << "vec4 GetPatchCoord() { return GetPatchCoord(0); }\n";

    _genGS << "vec4 GetPatchCoord(int localIndex);\n";
}

void
HdSt_CodeGen::_GenerateShaderParameters(bool bindlessTextureEnabled)
{
    /*
      ------------- Declarations -------------

      // shader parameter buffer
      struct ShaderData {
          <type>          <name>;
          vec4            diffuseColor;     // fallback uniform
          sampler2D       kdTexture;        // uv texture    (bindless texture)
          sampler2DArray  ptexTexels;       // ptex texels   (bindless texture)
          usamplerBuffer  ptexLayouts;      // ptex layouts  (bindless texture)
      };

      // bindless buffer
      layout (location=0) uniform ShaderData *shaderData;
      // not bindless buffer
      layout (std430, binding=0) buffer {
          ShaderData shaderData[];
      };

      // non bindless textures
      uniform sampler2D      samplers_2d[N];
      uniform sampler2DArray samplers_2darray[N];
      uniform isamplerBuffer isamplerBuffers[N];

      ------------- Accessors -------------

      * fallback value
      <type> HdGet_<name>(int localIndex=0) {
          return shaderData[GetDrawingCoord().shaderCoord].<name>
      }

      * primvar redirect
      <type> HdGet_<name>(int localIndex=0) {
          return HdGet_<inPrimvars>().xxx;
      }

      * bindless 2D texture
      <type> HdGet_<name>(int localIndex=0) {
          return texture(sampler2D(shaderData[GetDrawingCoord().shaderCoord].<name>), <inPrimvars>).xxx;
      }

      * non-bindless 2D texture
      <type> HdGet_<name>(int localIndex=0) {
          return texture(samplers_2d[<offset> + drawIndex * <stride>], <inPrimvars>).xxx;
      }

      * bindless Ptex texture
      <type> HdGet_<name>(int localIndex=0) {
          return GlopPtexTextureLookup(<name>_Data, <name>_Packing, GetPatchCoord()).xxx;
      }

      * non-bindless Ptex texture
      <type> HdGet_<name>(int localIndex=0) {
          return GlopPtexTextureLookup(
              samplers_2darray[<offset_ptex_texels> + drawIndex * <stride>],
              usamplerBuffers[<offset_ptex_layouts> + drawIndex * <stride>],
              GetPatchCoord()).xxx;
      }

      * bindless Ptex texture with patchcoord
      <type> HdGet_<name>(vec4 patchCoord) {
          return GlopPtexTextureLookup(<name>_Data, <name>_Packing, patchCoord).xxx;
      }

      * non-bindless Ptex texture
      <type> HdGet_<name>(vec4 patchCoord) {
          return GlopPtexTextureLookup(
              samplers_2darray[<offset_ptex_texels> + drawIndex * <stride>],
              usamplerBuffers[<offset_ptex_layouts> + drawIndex * <stride>],
              patchCoord).xxx;
      }

      * transform2d
      vec2 HdGet_<name>(int localIndex=0) {
          float angleRad = HdGet_<name>_rotation() * 3.1415926f / 180.f;
          mat2 rotMat = mat2(cos(angleRad), sin(angleRad), 
                             -sin(angleRad), cos(angleRad)); 
      #if defined(HD_HAS_<primvarName>)
          return vec2(HdGet_<name>_translation() + rotMat * 
            (HdGet_<name>_scale() * HdGet_<primvarName>(localIndex)));
      #else
          int shaderCoord = GetDrawingCoord().shaderCoord;
          return vec2(HdGet_<name>_translation() + rotMat * 
           (HdGet_<name>_scale() * shaderData[shaderCoord].<name>_fallback.xy));
      #endif
      }

    */

    std::stringstream declarations;
    std::stringstream accessors;

    TfToken typeName("ShaderData");
    TfToken varName("shaderData");

    // for shader parameters, we create declarations and accessors separetely.
    TF_FOR_ALL (it, _metaData.shaderData) {
        HdBinding binding = it->first;

        declarations << "struct " << typeName << " {\n";

        TF_FOR_ALL (dbIt, it->second.entries) {
            declarations << "  " << _GetPackedType(dbIt->dataType, false)
                         << " " << dbIt->name
                         << ";\n";

        }
        declarations << "};\n";

        // for array delaration, SSBO and bindless uniform can use [].
        // UBO requires the size [N].
        // XXX: [1] is a hack to cheat driver not telling the actual size.
        //      may not work some GPUs.
        // XXX: we only have 1 shaderData entry (interleaved).
        int arraySize = (binding.GetType() == HdBinding::UBO) ? 1 : 0;
        _EmitDeclaration(declarations, varName, typeName, binding, arraySize);

        break;
    }

    // Non-field redirect accessors.
    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {

        // adjust datatype
        std::string swizzle = _GetSwizzleString(it->second.dataType,
                                                it->second.swizzle);

        HdBinding::Type bindingType = it->first.GetType();
        if (bindingType == HdBinding::FALLBACK) {
            // vec4 HdGet_name(int localIndex)
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(int localIndex) {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return "
                << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(shaderData[shaderCoord]."
                << it->second.name << HdSt_ResourceBindingSuffixTokens->fallback
                << swizzle
                << ");\n"
                << "}\n";

            // vec4 HdGet_name()
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name
                << "() { return HdGet_" << it->second.name << "(0); }\n";

        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_2D) {
            // a function returning sampler requires bindless_texture

            _EmitTextureAccessors(
                accessors, it->second, swizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ true,
                /* isBindless = */ true,
                bindlessTextureEnabled);
        } else if (bindingType == HdBinding::BINDLESS_ARRAY_OF_TEXTURE_2D) {      
            // Handle special case for shadow textures.
            bool const isShadowTexture = 
                (it->second.name == HdStTokens->shadowCompareTextures);

            _EmitTextureAccessors(
                accessors, it->second, swizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ !isShadowTexture,
                /* isBindless = */ true,
                bindlessTextureEnabled,
                /* isArray = */ true,
                /* isShadowSampler = */ isShadowTexture);
        } else if (bindingType == HdBinding::TEXTURE_2D) {
            declarations
                << pxrImagingHdStCodeGen::LayoutQualifier(it->first)
                << "uniform sampler2D sampler2d_" << it->second.name << ";\n";

            _EmitTextureAccessors(
                accessors, it->second, swizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ true,
                /* isBindless = */ false,
                bindlessTextureEnabled);
        } else if (bindingType == HdBinding::ARRAY_OF_TEXTURE_2D) {       
            // Handle special case for shadow textures.
            bool const isShadowTexture = 
                (it->second.name == HdStTokens->shadowCompareTextures);

            declarations << "#define " << it->second.name << "_SIZE " 
                << it->second.arrayOfTexturesSize << "\n";
            if (isShadowTexture) {
                declarations
                    << pxrImagingHdStCodeGen::LayoutQualifier(it->first)
                    << "uniform sampler2DShadow sampler2d_" << it->second.name 
                    << "[" << it->second.name << "_SIZE];\n";
            } else {
                declarations
                    << pxrImagingHdStCodeGen::LayoutQualifier(it->first)
                    << "uniform sampler2D sampler2d_" << it->second.name 
                    << "[" << it->second.name << "_SIZE];\n";
            }

            _EmitTextureAccessors(
                accessors, it->second, swizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ !isShadowTexture,
                /* isBindless = */ false,
                bindlessTextureEnabled,
                /* isArray = */ true,
                /* isShadowSampler = */ isShadowTexture);
        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_FIELD) {

            _EmitTextureAccessors(
                accessors, it->second, swizzle,
                /* dim = */ 3,
                /* hasTextureTransform = */ true,
                /* hasTextureScaleAndBias = */ false,
                /* isBindless = */ true,
                bindlessTextureEnabled);

        } else if (bindingType == HdBinding::TEXTURE_FIELD) {

            declarations
                << pxrImagingHdStCodeGen::LayoutQualifier(it->first)
                << "uniform sampler3D sampler3d_" << it->second.name << ";\n";

            _EmitTextureAccessors(
                accessors, it->second, swizzle,
                /* dim = */ 3,
                /* hasTextureTransform = */ true,
                /* hasTextureScaleAndBias = */ false,
                /* isBindless = */ false,
                bindlessTextureEnabled);

        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_UDIM_ARRAY) {
            accessors 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->scale << "\n"
                << "vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->scale << "();\n"
                << "#endif\n"
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->bias << "\n"
                << "vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->bias << "();\n"
                << "#endif\n";
                
            // a function returning sampler requires bindless_texture
            if (bindlessTextureEnabled) {
                accessors
                    << "sampler2DArray\n"
                    << "HdGetSampler_" << it->second.name << "() {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  return sampler2DArray(shaderData[shaderCoord]."
                    << it->second.name << ");\n"
                    << "}\n";
            }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name << "()" << " {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord;\n";
            if (!it->second.inPrimvars.empty()) {
                accessors
                    << "  vec3 c = vec3(0.0, 0.0, 0.0);\n"
                    << "#if defined(HD_HAS_"
                    << it->second.inPrimvars[0] << ")\n"
                    << "  uvec2 handle = shaderData[shaderCoord]."
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->layout << ";\n"
                    << "  if (handle != uvec2(0)) {\n"
                    << "    c = hd_sample_udim(HdGet_"
                    << it->second.inPrimvars[0] << "().xy);\n"
                    << "    c.z = "
                    << "texelFetch(sampler1D(handle), int(c.z), 0).x - 1;\n"
                    << "  }\n"
                    << "#endif\n";
            } else {
                accessors
                    << "  vec3 c = vec3(0.0, 0.0, 0.0);\n";
            }
            accessors
                << "  vec4 ret = vec4(0, 0, 0, 0);\n"
                << "  if (c.z >= -0.5) {\n"
                << "    uvec2 handleTexels = shaderData[shaderCoord]."
                << it->second.name << ";\n"
                << "    if (handleTexels != uvec2(0)) {\n"
                << "      ret = texture(sampler2DArray(handleTexels), c);\n"
                << "    }\n  }\n"
                << "  return (ret\n"
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->scale << "\n"
                << "    * HdGet_" << it->second.name << "_" 
                << HdStTokens->scale << "()\n"
                << "#endif\n" 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->bias << "\n"
                << "    + HdGet_" << it->second.name << "_" 
                << HdStTokens->bias  << "()\n"
                << "#endif\n"
                << "  )" << swizzle << ";\n}\n";
                    
            // Create accessor for texture coordinates based on param name
            // vec2 HdGetCoord_name()
            accessors
                << "vec2 HdGetCoord_" << it->second.name << "() {\n"
                << "  return \n";
            if (!it->second.inPrimvars.empty()) {
                accessors 
                    << "#if defined(HD_HAS_" << it->second.inPrimvars[0] <<")\n"
                    << "  HdGet_" << it->second.inPrimvars[0] << "().xy;\n"
                    << "#else\n"
                    << "  vec2(0.0, 0.0)\n"
                    << "#endif\n";
            } else {
                accessors
                    << "  vec2(0.0, 0.0)\n";
            }
            accessors << "; }\n";  

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }      
        } else if (bindingType == HdBinding::TEXTURE_UDIM_ARRAY) {
            accessors 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->scale << "\n"
                << "vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->scale << "();\n"
                << "#endif\n"
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->bias << "\n"
                << "vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->bias << "();\n"
                << "#endif\n";

            declarations
                << pxrImagingHdStCodeGen::LayoutQualifier(it->first)
                << "uniform sampler2DArray sampler2dArray_"
                << it->second.name << ";\n";

            // a function returning sampler requires bindless_texture
            if (bindlessTextureEnabled) {
                accessors
                    << "sampler2DArray\n"
                    << "HdGetSampler_" << it->second.name << "() {\n"
                    << "  return sampler2dArray_" << it->second.name << ";"
                    << "}\n";
            } else {
                accessors
                    << "#define HdGetSampler_" << it->second.name << "()"
                    << " sampler2dArray_" << it->second.name << "\n";
            }
            // vec4 HdGet_name(vec2 coord) { vec3 c = hd_sample_udim(coord);
            // c.z = texelFetch(sampler1d_name_layout, int(c.z), 0).x - 1;
            // vec4 ret = vec4(0, 0, 0, 0);
            // if (c.z >= -0.5) { ret = texture(sampler2dArray_name, c); }
            // return (ret
            // #ifdef HD_HAS_name_scale
            //   * HdGet_name_scale()
            // #endif
            // #ifdef HD_HAS_name_bias
            //   + HdGet_name_bias()
            // #endif
            // ).xyz; }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "(vec2 coord) { vec3 c = hd_sample_udim(coord);\n"
                << "  c.z = texelFetch(sampler1d_"
                << it->second.name << HdSt_ResourceBindingSuffixTokens->layout
                << ", int(c.z), 0).x - 1;\n"
                << "  vec4 ret = vec4(0, 0, 0, 0);\n"
                << "  if (c.z >= -0.5) { ret = texture(sampler2dArray_"
                << it->second.name << ", c); }\n  return (ret\n"
                << "#ifdef HD_HAS_" << it->second.name << "_"
                << HdStTokens->scale << "\n"
                << "    * HdGet_" << it->second.name << "_" 
                << HdStTokens->scale << "()\n"
                << "#endif\n" 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->bias << "\n"
                << "    + HdGet_" << it->second.name << "_" 
                << HdStTokens->bias  << "()\n"
                << "#endif\n"
                << "  )" << swizzle << ";\n}\n";

            // Create accessor for texture coordinates based on param name
            // vec2 HdGetCoord_name()
            accessors
                << "vec2 HdGetCoord_" << it->second.name << "() {\n"
                << "  return \n";
            if (!it->second.inPrimvars.empty()) {
                accessors 
                    << "#if defined(HD_HAS_" << it->second.inPrimvars[0] <<")\n"
                    << "  HdGet_" << it->second.inPrimvars[0] << "().xy\n"
                    << "#else\n"
                    << "  vec2(0.0, 0.0)\n"
                    << "#endif\n";
            } else {
                accessors
                    << "  vec2(0.0, 0.0)\n";
            }
            accessors << "; }\n";

            // vec4 HdGet_name() { return HdGet_name(HdGetCoord_name()); }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "() { return HdGet_" << it->second.name << "("
                << "HdGetCoord_" << it->second.name << "()); }\n";

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }
        } else if (bindingType == HdBinding::TEXTURE_UDIM_LAYOUT) {
            declarations
                << pxrImagingHdStCodeGen::LayoutQualifier(it->first)
                << "uniform sampler1D sampler1d_" << it->second.name << ";\n";
        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL) {
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(int localIndex) {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return " << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(PtexTextureLookup("
                << "sampler2DArray(shaderData[shaderCoord]."
                << it->second.name << "),"
                << "usampler1DArray(shaderData[shaderCoord]."
                << it->second.name << HdSt_ResourceBindingSuffixTokens->layout
                <<"), "
                << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                << "}\n"
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "()"
                << "{ return HdGet_" << it->second.name << "(0); }\n"
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return " << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(PtexTextureLookup("
                << "sampler2DArray(shaderData[shaderCoord]."
                << it->second.name << "),"
                << "usampler1DArray(shaderData[shaderCoord]."
                << it->second.name << HdSt_ResourceBindingSuffixTokens->layout
                << "), "
                << "patchCoord)" << swizzle << ");\n"
                << "}\n";

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }     
        } else if (bindingType == HdBinding::TEXTURE_PTEX_TEXEL) {
            declarations
                << pxrImagingHdStCodeGen::LayoutQualifier(it->first)
                << "uniform sampler2DArray sampler2darray_"
                << it->second.name << ";\n";
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(int localIndex) {\n"
                << "  return " << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(PtexTextureLookup("
                << "sampler2darray_" << it->second.name << ", "
                << "usampler1darray_"
                << it->second.name << HdSt_ResourceBindingSuffixTokens->layout
                << ", "
                << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                << "}\n"
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "()"
                << "{ return HdGet_" << it->second.name << "(0); }\n"
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                << "  return " << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(PtexTextureLookup("
                << "sampler2darray_" << it->second.name << ", "
                << "usampler1darray_"
                << it->second.name << HdSt_ResourceBindingSuffixTokens->layout
                << ", "
                << "patchCoord)" << swizzle << ");\n"
                << "}\n";

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }    
        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT) {
            //accessors << _GetUnpackedType(it->second.dataType) << "(0)";
        } else if (bindingType == HdBinding::TEXTURE_PTEX_LAYOUT) {
            declarations
                << pxrImagingHdStCodeGen::LayoutQualifier(HdBinding(it->first.GetType(),
                                             it->first.GetLocation(),
                                             it->first.GetTextureUnit()))
                << "uniform usampler1DArray usampler1darray_"
                << it->second.name << ";\n";
        } else if (bindingType == HdBinding::PRIMVAR_REDIRECT) {
            // Create an HdGet_INPUTNAME for the shader to access a primvar
            // for which a HdGet_PRIMVARNAME was already generated earlier.
            
            // XXX: shader and primvar name collisions are a problem!
            // (see, e.g., HYD-1800).
            if (it->second.name == it->second.inPrimvars[0]) {
                // Avoid the following:
                // If INPUTNAME and PRIMVARNAME are the same and the
                // primvar exists, we would generate two functions
                // both called HdGet_PRIMVAR, one to read the primvar
                // (based on _metaData.constantData) and one for the
                // primvar redirect here.
                accessors
                    << "#if !defined(HD_HAS_" << it->second.name << ")\n";
            }
            
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "() {\n"
                // If primvar exists, use it
                << "#if defined(HD_HAS_" << it->second.inPrimvars[0] << ")\n"
                << "  return HdGet_" << it->second.inPrimvars[0] << "();\n"
                << "#else\n"
                // Otherwise use default value.
                << "  int shaderCoord = GetDrawingCoord().shaderCoord;\n"
                << "  return "
                << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(shaderData[shaderCoord]."
                << it->second.name << HdSt_ResourceBindingSuffixTokens->fallback
                << swizzle <<  ");\n"
                << "#endif\n"
                << "\n}\n"
                << "#define HD_HAS_" << it->second.name << " 1\n";
            
            if (it->second.name == it->second.inPrimvars[0]) {
                accessors
                    << "#endif\n";
            }
        } else if (bindingType == HdBinding::TRANSFORM_2D) {
            // Forward declare rotation, scale, and translation
            accessors 
                << "float HdGet_" << it->second.name << "_" 
                << HdStTokens->rotation  << "();\n"
                << "vec2 HdGet_" << it->second.name << "_" 
                << HdStTokens->scale  << "();\n"
                << "vec2 HdGet_" << it->second.name << "_" 
                << HdStTokens->translation  << "();\n";

            // vec2 HdGet_name(int localIndex)
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(int localIndex) {\n"
                << "  float angleRad = HdGet_" << it->second.name << "_" 
                << HdStTokens->rotation  << "()"
                << " * 3.1415926f / 180.f;\n"
                << "  mat2 rotMat = mat2(cos(angleRad), sin(angleRad), "
                << "-sin(angleRad), cos(angleRad)); \n";
            // If primvar exists, use it
            if (!it->second.inPrimvars.empty()) {
                accessors
                    << "#if defined(HD_HAS_" << it->second.inPrimvars[0] << ")\n"
                    << "  return vec2(HdGet_" << it->second.name << "_" 
                    << HdStTokens->translation << "() + rotMat * (HdGet_" 
                    << it->second.name << "_" << HdStTokens->scale << "() * "
                    << "HdGet_" << it->second.inPrimvars[0] << "(localIndex)));\n"
                    << "#else\n";
            }
            // Otherwise use default value.
            accessors
                << "  int shaderCoord = GetDrawingCoord().shaderCoord;\n"
                << "  return vec2(HdGet_" << it->second.name << "_" 
                << HdStTokens->translation << "() + rotMat * (HdGet_" 
                << it->second.name << "_" << HdStTokens->scale << "() * "
                << "shaderData[shaderCoord]." << it->second.name 
                << HdSt_ResourceBindingSuffixTokens->fallback << swizzle 
                << "));\n";
            if (!it->second.inPrimvars.empty()) {
                accessors << "#endif\n"; 
            }
            accessors << "}\n";

            // vec2 HdGet_name()
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "() {\n"
                << "  return HdGet_" << it->second.name << "(0);\n"
                << "}\n";
        }
    }

    
    accessors
        << "void ProcessSamplingTransforms("
        << "MAT4 instanceModelViewInverse) {\n";

    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {
        const HdBinding::Type bindingType = it->first.GetType();

        if ( bindingType == HdBinding::TEXTURE_FIELD ||
             bindingType == HdBinding::BINDLESS_TEXTURE_FIELD) {

            const std::string eyeToSamplingTransform =
                "eyeTo" + it->second.name.GetString() + "SamplingTransform";
            
            accessors
                << "    Process_" << eyeToSamplingTransform
                << "(instanceModelViewInverse);\n";
        }
    }

    accessors
        << "}\n";

    // Field redirect accessors, need to access above field textures.
    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {
        HdBinding::Type bindingType = it->first.GetType();

        if (bindingType == HdBinding::FIELD_REDIRECT) {

            // adjust datatype
            std::string swizzle = _GetSwizzleString(it->second.dataType);

            const TfToken fieldName =
                it->second.inPrimvars.empty()
                ? TfToken("FIELDNAME_WAS_NOT_SPECIFIED")
                : it->second.inPrimvars[0];

            // Create an HdGet_INPUTNAME(vec3) for the shader to access a 
            // field texture HdGet_FIELDNAMETexture(vec3).
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(vec3 coord) {\n"
                // If field exists, use it
                << "#if defined(HD_HAS_"
                << fieldName << HdSt_ResourceBindingSuffixTokens->texture
                << ")\n"
                << "  return HdGet_"
                << fieldName << HdSt_ResourceBindingSuffixTokens->texture
                << "(coord)"
                << swizzle << ";\n"
                << "#else\n"
                // Otherwise use default value.
                << "  int shaderCoord = GetDrawingCoord().shaderCoord;\n"
                << "  return "
                << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(shaderData[shaderCoord]."
                << it->second.name << HdSt_ResourceBindingSuffixTokens->fallback
                <<  ");\n"
                << "#endif\n"
                << "\n}\n";
        }
    }

    _genFS << declarations.str()
           << accessors.str();

    _genGS << declarations.str()
           << accessors.str();
}

void
HdSt_CodeGen::_GenerateTopologyVisibilityParameters()
{
    std::stringstream declarations;
    std::stringstream accessors;
    TF_FOR_ALL (it, _metaData.topologyVisibilityData) {
        // See note in _GenerateConstantPrimvar re: padding.
        HdBinding binding = it->first;
        TfToken typeName(TfStringPrintf("TopologyVisibilityData%d",
                                        binding.GetValue()));
        TfToken varName = it->second.blockName;

        declarations << "struct " << typeName << " {\n";

        TF_FOR_ALL (dbIt, it->second.entries) {
            if (!TF_VERIFY(!dbIt->dataType.IsEmpty(),
                              "Unknown dataType for %s",
                              dbIt->name.GetText())) {
                continue;
            }

            declarations << "  " << _GetPackedType(dbIt->dataType, false)
                         << " " << dbIt->name;
            if (dbIt->arraySize > 1) {
                declarations << "[" << dbIt->arraySize << "]";
            }

            declarations << ";\n";

            _EmitStructAccessor(accessors, varName, dbIt->name, dbIt->dataType,
                                dbIt->arraySize,
                                "GetDrawingCoord().topologyVisibilityCoord");
        }
        declarations << "};\n";

        _EmitDeclaration(declarations, varName, typeName, binding,
                         /*arraySize=*/1);
    }
    _genCommon << declarations.str()
               << accessors.str();
}

PXR_NAMESPACE_CLOSE_SCOPE

