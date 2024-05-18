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
#include "pxr/imaging/hdSt/codeGen.h"
#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hdSt/hioConversions.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hgi/capabilities.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hio/glslfxResourceLayout.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/base/tf/hash.h"

#include <sstream>
#include <unordered_map>

#if defined(__APPLE__)
#include <opensubdiv/osd/mtlPatchShaderSource.h>
#else
#include <opensubdiv/osd/glslPatchShaderSource.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_double, "double"))
    ((_float, "float"))
    ((_int, "int"))
    ((_uint, "uint"))
    ((_bool, "bool"))
    ((_atomic_int, "atomic_int"))
    ((_atomic_uint, "atomic_uint"))
    ((_default, "default"))
    (flat)
    (noperspective)
    (sample)
    (centroid)
    (patch)
    (hd_barycentricCoord)
    (hd_patchID)
    (hd_tessCoord)
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
    (PrimvarData)
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
    (gl_MaxPatchVertices)
    (HD_NUM_PATCH_EVAL_VERTS)
    (HD_NUM_PRIMITIVE_VERTS)
    (quads)
    (isolines)
    (equal_spacing)
    (fractional_even_spacing)
    (fractional_odd_spacing)
    (cw)
    (ccw)
    (points)
    (lines)
    (lines_adjacency)
    (triangles)
    (triangles_adjacency)
    (line_strip)
    (triangle_strip)
    (early_fragment_tests)
    (OsdPerPatchVertexBezier)
);

TF_DEFINE_ENV_SETTING(HDST_ENABLE_HGI_RESOURCE_GENERATION, false,
                      "Enable Hgi resource generation for codeGen");

/* static */
bool
HdSt_CodeGen::IsEnabledHgiResourceGeneration(Hgi const *hgi)
{
    static bool const isEnabled =
        TfGetEnvSetting(HDST_ENABLE_HGI_RESOURCE_GENERATION);
    
    TfToken const& hgiName = hgi->GetAPIName();

    // Check if is env var is true, otherwise return true if NOT using HgiGL, 
    // as Hgi resource generation is required for Metal and Vulkan.
    return isEnabled || hgiName != HgiTokens->OpenGL;
}

HdSt_CodeGen::HdSt_CodeGen(
    HdSt_GeometricShaderPtr const &geometricShader,
    HdStShaderCodeSharedPtrVector const &shaders,
    TfToken const &materialTag,
    std::unique_ptr<HdSt_ResourceBinder::MetaData>&& metaData)
    : _metaData(std::move(metaData))
    , _geometricShader(geometricShader)
    , _shaders(shaders)
    , _materialTag(materialTag)
    , _hasVS(false)
    , _hasTCS(false)
    , _hasTES(false)
    , _hasGS(false)
    , _hasFS(false)
    , _hasCS(false)
    , _hasPTCS(false)
    , _hasPTVS(false)
    , _hasClipPlanes(false)
{
    TF_VERIFY(geometricShader);
    TF_VERIFY(_metaData, 
              "Invalid MetaData ptr passed in as constructor arg.");
}

HdSt_CodeGen::HdSt_CodeGen(
    HdStShaderCodeSharedPtrVector const &shaders,
    std::unique_ptr<HdSt_ResourceBinder::MetaData>&& metaData)
    : _metaData(std::move(metaData))
    , _geometricShader()
    , _shaders(shaders)
    , _hasVS(false)
    , _hasTCS(false)
    , _hasTES(false)
    , _hasGS(false)
    , _hasFS(false)
    , _hasCS(false)
    , _hasPTCS(false)
    , _hasPTVS(false)
    , _hasClipPlanes(false)
{
    TF_VERIFY(_metaData,
              "Invalid MetaData ptr passed in as constructor arg.");
}

HdSt_CodeGen::ID
HdSt_CodeGen::ComputeHash() const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_metaData,
                   "Metadata not properly initialized by resource binder.")) {
        return {}; 
    }

    return TfHash::Combine(
        _geometricShader ? _geometricShader->ComputeHash() : 0,
        _metaData->ComputeHash(),
        HdStShaderCode::ComputeHash(_shaders),
        _materialTag.Hash()
    );
}

static
std::string
_GetPtexTextureShaderSource()
{
    static std::string source =
        HioGlslfx(HdStPackagePtexTextureShader()).GetSource(
            _tokens->ptexTextureSampler);
    return source;
}

// TODO: Shuffle code to remove these declarations.
static void _EmitDeclaration(HioGlslfxResourceLayout::ElementVector *elements,
                             TfToken const &name,
                             TfToken const &type,
                             HdStBinding const &binding,
                             bool isWritable=false,
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
                                 HdStBinding const &binding,
                                 const char *index);

static void _EmitComputeMutator(std::stringstream &str,
                                TfToken const &name,
                                TfToken const &type,
                                HdStBinding const &binding,
                                const char *index);

static void _EmitAccessor(std::stringstream &str,
                          TfToken const &name,
                          TfToken const &type,
                          HdStBinding const &binding,
                          const char *index=NULL);

static void _EmitScalarAccessor(std::stringstream &str,
                                TfToken const &name,
                                TfToken const &type);
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
    return
           "// Alias hgi vec and matrix types to hd.\n"
           "#define hd_ivec3 hgi_ivec3\n"
           "#define hd_vec3 hgi_vec3\n"
           "#define hd_dvec3 hgi_dvec3\n"
           "#define hd_mat3 hgi_mat3\n"
           "#define hd_dmat3 hgi_dmat3\n"
           "\n"
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
        if (token == _tokens->ivec3) {
            return _tokens->hd_ivec3;
        } else if (token == _tokens->vec3) {
            return _tokens->hd_vec3;
        } else if (token == _tokens->dvec3) {
            return _tokens->hd_dvec3;
        } else if (token == _tokens->mat3) {
            return _tokens->hd_mat3;
        } else if (token == _tokens->dmat3) {
            return _tokens->hd_dmat3;
        }
    }
    if (token == _tokens->packed_2_10_10_10) {
        return _tokens->_int;
    }
    if (token == _tokens->packed_half2) {
        return _tokens->_uint;
    }
    if (token == _tokens->packed_half4) {
        return _tokens->uvec2;
    }
    return token;
}

static TfToken const &
_GetUnpackedType(TfToken const &token, bool packedAlignment)
{
    if (token == _tokens->packed_2_10_10_10) {
        return _tokens->vec4;
    }
    if (token == _tokens->packed_half2) {
        return _tokens->vec2;
    }
    if (token == _tokens->packed_half4) {
        return _tokens->vec4;
    }
    return token;
}

static TfToken const &
_GetPackedTypeAccessor(TfToken const &token, bool packedAlignment)
{
    if (packedAlignment) {
        if (token == _tokens->ivec3) {
            return _tokens->hd_ivec3_get;
        } else if (token == _tokens->vec3) {
            return _tokens->hd_vec3_get;
        } else if (token == _tokens->dvec3) {
            return _tokens->hd_dvec3_get;
        } else if (token == _tokens->mat3) {
            return _tokens->hd_mat3_get;
        } else if (token == _tokens->dmat3) {
            return _tokens->hd_dmat3_get;
        }
    }
    if (token == _tokens->packed_2_10_10_10) {
        return _tokens->hd_vec4_2_10_10_10_get;
    }
    if (token == _tokens->packed_half2) {
        return _tokens->hd_half2_get;
    }
    if (token == _tokens->packed_half4) {
        return _tokens->hd_half4_get;
    }
    return token;
}

static TfToken const &
_GetPackedTypeMutator(TfToken const &token, bool packedAlignment)
{
    if (packedAlignment) {
        if (token == _tokens->ivec3) {
            return _tokens->hd_ivec3_set;
        } else if (token == _tokens->vec3) {
            return _tokens->hd_vec3_set;
        } else if (token == _tokens->dvec3) {
            return _tokens->hd_dvec3_set;
        } else if (token == _tokens->mat3) {
            return _tokens->hd_mat3_set;
        } else if (token == _tokens->dmat3) {
            return _tokens->hd_dmat3_set;
        }
    }
    if (token == _tokens->packed_2_10_10_10) {
        return _tokens->hd_vec4_2_10_10_10_set;
    }
    if (token == _tokens->packed_half2) {
        return _tokens->hd_half2_set;
    }
    if (token == _tokens->packed_half4) {
        return _tokens->hd_half4_set;
    }
    return token;
}

static TfToken const &
_GetFlatType(TfToken const &token)
{
    if (token == _tokens->ivec2) {
        return _tokens->_int;
    } else if (token == _tokens->ivec3) {
        return _tokens->_int;
    } else if (token == _tokens->ivec4) {
        return _tokens->_int;
    } else if (token == _tokens->vec2) {
        return _tokens->_float;
    } else if (token == _tokens->vec3) {
        return _tokens->_float;
    } else if (token == _tokens->vec4) {
        return _tokens->_float;
    } else if (token == _tokens->dvec2) {
        return _tokens->_double;
    } else if (token == _tokens->dvec3) {
        return _tokens->_double;
    } else if (token == _tokens->dvec4) {
        return _tokens->_double;
    } else if (token == _tokens->mat3) {
        return _tokens->_float;
    } else if (token == _tokens->mat4) {
        return _tokens->_float;
    } else if (token == _tokens->dmat3) {
        return _tokens->_double;
    } else if (token == _tokens->dmat4) {
        return _tokens->_double;
    } else if (token == _tokens->packed_2_10_10_10) {
        return _tokens->_float;
    } else if (token == _tokens->packed_half2) {
        return _tokens->_float;
    } else if (token == _tokens->packed_half4) {
        return _tokens->_float;
    }
    return token;
}

static std::string
_GetFlatTypeSwizzleString(TfToken const &token)
{
    if (token == _tokens->ivec2 ||
        token == _tokens->ivec3 ||
        token == _tokens->ivec4 ||
        token == _tokens->uvec2 ||
        token == _tokens->uvec3 ||
        token == _tokens->uvec4 ||
        token == _tokens->vec2 ||
        token == _tokens->vec3 ||
        token == _tokens->vec4 ||
        token == _tokens->dvec2 ||
        token == _tokens->dvec3 ||
        token == _tokens->dvec4 ||
        token == _tokens->packed_2_10_10_10 ||
        token == _tokens->packed_half2 ||
        token == _tokens->packed_half4) {
        return ".x";
    }
    return "";
}

static TfToken const &
_ConvertBoolType(TfToken const &token)
{
    if (token == _tokens->_bool) {
        return _tokens->_int;
    }
    return token;
}

namespace {

using InOut = HioGlslfxResourceLayout::InOut;
using Kind = HioGlslfxResourceLayout::Kind;
using TextureType = HioGlslfxResourceLayout::TextureType;

class _ResourceGenerator
{
public:
    _ResourceGenerator()
        : _interstageSlotTable()
        , _nextInterstageSlot(0)
        , _outputSlotTable()
        , _nextOutputSlot(0)
        , _nextOutputLocation(0)
        { }
    ~_ResourceGenerator() = default;

    void _GenerateHgiResources(
        Hgi const * hgi,
        HgiShaderFunctionDesc *funcDesc,
        TfToken const & shaderStage,
        HioGlslfxResourceLayout::ElementVector const & elements,
        HdSt_ResourceBinder::MetaData const & metaData);

    void _GenerateHgiTextureResources(
        HgiShaderFunctionDesc *funcDesc,
        TfToken const & shaderStage,
        HioGlslfxResourceLayout::TextureElementVector const & textureElements,
        HdSt_ResourceBinder::MetaData const & metaData);

    void _GenerateGLSLResources(
        HgiShaderFunctionDesc *funcDesc,
        std::stringstream &str,
        TfToken const &shaderStage,
        HioGlslfxResourceLayout::ElementVector const & elements,
        HdSt_ResourceBinder::MetaData const & metaData);

    void _GenerateGLSLTextureResources(
        std::stringstream &str,
        TfToken const &shaderStage,
        HioGlslfxResourceLayout::TextureElementVector const & textureElements,
        HdSt_ResourceBinder::MetaData const & metaData);

    void  _AdvanceShaderStage()
    {
        // Reset interstage slot counter when moving to next stage.
        _nextInterstageSlot = 0;
    }

private:
    using _SlotTable = std::unordered_map<TfToken, int32_t, TfHash>;

    int32_t _GetLocation(
        HioGlslfxResourceLayout::Element const &element,
        HdSt_ResourceBinder::MetaData const & metaData)
    {
        if (element.location >= 0) {
            return static_cast<uint32_t>(element.location);
        }
        for (auto const & custom : metaData.customBindings) {
            if (custom.name == element.name) {
                return custom.binding.GetLocation();
            }
        }

        return _nextOutputLocation++;
    }

    int32_t _GetSlot(TfToken const & name, bool in, uint32_t const count = 1)
    {
        _SlotTable::iterator entry = _interstageSlotTable.find(name);

        // For input interstage slots, check slot table.
        if (in && entry != _interstageSlotTable.end()) {
            return entry->second;
        }

        int32_t const currentSlot = _nextInterstageSlot;
        _interstageSlotTable.insert_or_assign(name, currentSlot);
        _nextInterstageSlot += count;

        return currentSlot;
    }

    int32_t _GetOutputSlot(TfToken const & name, uint32_t const count = 1)
    {
        _SlotTable::iterator entry = _outputSlotTable.find(name);

        if (entry != _outputSlotTable.end()) {
            return entry->second;
        }

        int32_t const currentSlot = _nextOutputSlot;
        _outputSlotTable.insert({name, currentSlot});
        _nextOutputSlot += count;

        return currentSlot;
    }

    TfToken _GetFlattenedName(TfToken const & aggregateName,
                              TfToken const & memberName) const
    {
        return TfToken(
                aggregateName.GetString() + "_" + memberName.GetString());
    }

    HgiInterpolationType _GetInterpolation(TfToken const & qualifiers) const
    {
        if (qualifiers == _tokens->flat) {
            return HgiInterpolationFlat;
        } else if (qualifiers == _tokens->noperspective) {
            return HgiInterpolationNoPerspective;
        } else {
            return HgiInterpolationDefault;
        }
    }

    HgiSamplingType _GetSamplingQualifier(TfToken const & qualifiers) const
    {
        if (qualifiers == _tokens->centroid) {
            return HgiSamplingCentroid;
        } else if (qualifiers == _tokens->sample) {
            return HgiSamplingSample;
        } else {
            return HgiSamplingDefault;
        }
    }

    HgiStorageType _GetStorageQualifier(TfToken const & qualifiers) const
    {
        if (qualifiers == _tokens->patch) {
            return HgiStoragePatch;
        } else {
            return HgiStorageDefault;
        }
    }

    std::string _GetOutputRoleName(TfToken const & outputName)
    {
        return "color(" + std::to_string(_GetOutputSlot(outputName)) + ")";
    }

    _SlotTable _interstageSlotTable;
    uint32_t _nextInterstageSlot;
    _SlotTable _outputSlotTable;
    uint32_t _nextOutputSlot;
    uint32_t _nextOutputLocation;
};

bool
_IsVertexAttribInputStage(TfToken const & shaderStage)
{
    return (shaderStage == HdShaderTokens->vertexShader) ||
           (shaderStage == HdShaderTokens->postTessControlShader) ||
           (shaderStage == HdShaderTokens->postTessVertexShader);
}

// Most data types we use in Storm take up one location slot. There are some 
// exceptions, which we can add here as we find them.
uint32_t
_GetLocationCount(Hgi const * hgi, TfToken const & dataType)
{
    if (dataType == _tokens->OsdPerPatchVertexBezier) {
        return 2;
    }
    
    // In Vulkan, 64-bit three or four component vectors take up two slots.
    if ((dataType == _tokens->dvec3 || dataType == _tokens->dvec4) && 
        hgi->GetAPIName() == HgiTokens->Vulkan) {
        return 2;
    }
    
    return 1;
}

void
_ResourceGenerator::_GenerateHgiResources(
    Hgi const * hgi,
    HgiShaderFunctionDesc *funcDesc,
    TfToken const & shaderStage,
    HioGlslfxResourceLayout::ElementVector const & elements,
    HdSt_ResourceBinder::MetaData const & metaData)
{
    using InOut = HioGlslfxResourceLayout::InOut;
    using Kind = HioGlslfxResourceLayout::Kind;

    for (auto const & element : elements) {
        if (element.kind == Kind::VALUE) {
            if (element.inOut == InOut::STAGE_IN) {
                if (_IsVertexAttribInputStage(shaderStage)) {
                    HgiShaderFunctionParamDesc param;
                    param.nameInShader = element.name;
                    param.type = element.dataType;
                    param.location = _GetLocation(element, metaData);
                    if (shaderStage == HdShaderTokens->postTessControlShader ||
                        shaderStage == HdShaderTokens->postTessVertexShader) {
                        param.arraySize = "VERTEX_CONTROL_POINTS_PER_PATCH";
                    }
                    HgiShaderFunctionAddStageInput(funcDesc, param);
                } else {
                    HgiShaderFunctionParamDesc param;
                    param.nameInShader = element.name;
                    param.type = element.dataType;
                    param.interstageSlot = _GetSlot(element.name, /*in*/true,
                        _GetLocationCount(hgi, element.dataType));
                    param.interpolation = _GetInterpolation(element.qualifiers);
                    param.sampling = _GetSamplingQualifier(element.qualifiers);
                    param.storage = _GetStorageQualifier(element.qualifiers);
                    param.arraySize = element.arraySize;
                    HgiShaderFunctionAddStageInput(funcDesc, param);
                }
            } else if (element.inOut == InOut::STAGE_OUT) {
                if (shaderStage == HdShaderTokens->fragmentShader) {
                    HgiShaderFunctionAddStageOutput(
                        funcDesc,
                        /*name=*/element.name,
                        /*type=*/element.dataType,
                        /*role=*/_GetOutputRoleName(element.name));
                } else {
                    HgiShaderFunctionParamDesc param;
                    param.nameInShader = element.name,
                    param.type = element.dataType,
                    param.interstageSlot = _GetSlot(element.name, /*in*/false,
                        _GetLocationCount(hgi, element.dataType));
                    param.interpolation = _GetInterpolation(element.qualifiers);
                    param.sampling = _GetSamplingQualifier(element.qualifiers);
                    param.storage = _GetStorageQualifier(element.qualifiers);
                    param.arraySize = element.arraySize;
                    HgiShaderFunctionAddStageOutput(funcDesc, param);
                }
            }
        } else if (element.kind == Kind::BLOCK) {
            HgiShaderFunctionParamBlockDesc paramBlock;
            paramBlock.blockName = element.aggregateName;
            paramBlock.instanceName = element.name;
            paramBlock.arraySize = element.arraySize;

            TfToken const firstMemberBlockName =
                _GetFlattenedName(element.aggregateName,
                                  element.members.front().name);

            uint32_t locationCount = 0;
            for (const auto& member : element.members) {
                locationCount += _GetLocationCount(hgi, member.dataType);
            }
            paramBlock.interstageSlot = _GetSlot(firstMemberBlockName,
                /*in*/element.inOut == InOut::STAGE_IN, locationCount);

            for (auto const & member : element.members) {
                HgiShaderFunctionParamBlockDesc::Member paramMember;
                paramMember.name = member.name;
                paramMember.type = _ConvertBoolType(member.dataType);
                paramMember.interpolation = _GetInterpolation(member.qualifiers); 
                paramMember.sampling = _GetSamplingQualifier(member.qualifiers); 
                paramBlock.members.push_back(paramMember);
            }
            if (element.inOut == InOut::STAGE_IN) {
                funcDesc->stageInputBlocks.push_back(paramBlock);
            } else {
                funcDesc->stageOutputBlocks.push_back(paramBlock);
            }
        } else if (element.kind == Kind::QUALIFIER) {
            if (shaderStage == HdShaderTokens->tessControlShader) {
                if (element.inOut == InOut::STAGE_OUT) {
                    funcDesc->tessellationDescriptor.numVertsPerPatchOut =
                        element.qualifiers.GetString();
                }
            } else if (shaderStage == HdShaderTokens->tessEvalShader ||
                       shaderStage == HdShaderTokens->postTessVertexShader) {
                if (element.inOut == InOut::STAGE_IN) {
                    if (element.qualifiers == _tokens->triangles) {
                        funcDesc->tessellationDescriptor.patchType =
                            HgiShaderFunctionTessellationDesc::
                                PatchType::Triangles;
                    } else if (element.qualifiers == _tokens->quads) {
                        funcDesc->tessellationDescriptor.patchType =
                            HgiShaderFunctionTessellationDesc::
                                PatchType::Quads;
                    } else if (element.qualifiers == _tokens->isolines) {
                        funcDesc->tessellationDescriptor.patchType =
                            HgiShaderFunctionTessellationDesc::
                                PatchType::Isolines;
                    } else if (element.qualifiers ==
                                        _tokens->equal_spacing) {
                        funcDesc->tessellationDescriptor.spacing =
                            HgiShaderFunctionTessellationDesc::
                                Spacing::Equal;
                    } else if (element.qualifiers ==
                                        _tokens->fractional_even_spacing) {
                        funcDesc->tessellationDescriptor.spacing =
                            HgiShaderFunctionTessellationDesc::
                                Spacing::FractionalEven;
                    } else if (element.qualifiers ==
                                        _tokens->fractional_odd_spacing) {
                        funcDesc->tessellationDescriptor.spacing =
                            HgiShaderFunctionTessellationDesc::
                                Spacing::FractionalOdd;
                    } else if (element.qualifiers == _tokens->cw) {
                        funcDesc->tessellationDescriptor.ordering =
                            HgiShaderFunctionTessellationDesc::
                                Ordering::CW;
                    } else if (element.qualifiers == _tokens->ccw) {
                        funcDesc->tessellationDescriptor.ordering =
                            HgiShaderFunctionTessellationDesc::
                                Ordering::CCW;
                    }
                }
            } else if (shaderStage == HdShaderTokens->geometryShader) {
                if (element.inOut == InOut::STAGE_IN) {
                    if (element.qualifiers == _tokens->points) {
                        funcDesc->geometryDescriptor.inPrimitiveType =
                            HgiShaderFunctionGeometryDesc::InPrimitiveType::
                            Points;
                    } else if (element.qualifiers == _tokens->lines) {
                        funcDesc->geometryDescriptor.inPrimitiveType =
                            HgiShaderFunctionGeometryDesc::InPrimitiveType::
                            Lines;
                    } else if (element.qualifiers == _tokens->lines_adjacency) {
                        funcDesc->geometryDescriptor.inPrimitiveType =
                            HgiShaderFunctionGeometryDesc::InPrimitiveType::
                            LinesAdjacency;
                    } else if (element.qualifiers == _tokens->triangles) {
                        funcDesc->geometryDescriptor.inPrimitiveType =
                            HgiShaderFunctionGeometryDesc::InPrimitiveType::
                            Triangles;
                    } else if (element.qualifiers ==
                        _tokens->triangles_adjacency) {
                        funcDesc->geometryDescriptor.inPrimitiveType =
                            HgiShaderFunctionGeometryDesc::InPrimitiveType::
                            TrianglesAdjacency;
                    }
                } else if (element.inOut == InOut::STAGE_OUT) {
                    if (element.qualifiers == _tokens->points) {
                        funcDesc->geometryDescriptor.outPrimitiveType =
                            HgiShaderFunctionGeometryDesc::OutPrimitiveType::
                            Points;
                    } else if (element.qualifiers == _tokens->line_strip) {
                        funcDesc->geometryDescriptor.outPrimitiveType =
                            HgiShaderFunctionGeometryDesc::OutPrimitiveType::
                            LineStrip;
                    } else if (element.qualifiers == _tokens->triangle_strip) {
                        funcDesc->geometryDescriptor.outPrimitiveType =
                            HgiShaderFunctionGeometryDesc::OutPrimitiveType::
                            TriangleStrip;
                    } else {
                        // Assume any other GS stage out qualifier will be the
                        // number of max vertices.
                        funcDesc->geometryDescriptor.outMaxVertices = 
                            element.qualifiers.GetString();
                    }
                }
            } else if (element.qualifiers == _tokens->early_fragment_tests) {
                //   GLSL: "layout (early_fragment_tests) in;"
                //   MSL: "[[early_fragment_tests]]"
                funcDesc->fragmentDescriptor.earlyFragmentTests = true;
            }
        } else if (element.kind == Kind::UNIFORM_BLOCK) {
            if (TF_VERIFY(element.members.size() == 1)) {
                auto const & member = element.members.front();
                uint32_t const arraySize =
                    (element.arraySize.IsEmpty() ? 0 :
                        static_cast<uint32_t>(std::stoi(element.arraySize)));

                if (arraySize > 0) {
                    HgiShaderFunctionAddBuffer(
                        funcDesc,
                        /*name=*/member.name,
                        /*type=*/_ConvertBoolType(member.dataType),
                        /*bindIndex=*/_GetLocation(element, metaData),
                        /*binding=*/HgiBindingTypeUniformArray,
                        /*arraySize=*/arraySize);
                } else {
                    HgiShaderFunctionAddBuffer(
                        funcDesc,
                        /*name=*/member.name,
                        /*type=*/_ConvertBoolType(member.dataType),
                        /*bindIndex=*/_GetLocation(element, metaData),
                        /*binding=*/HgiBindingTypeUniformValue);
                }
            }
        } else if (element.kind == Kind::UNIFORM_BLOCK_CONSTANT_PARAMS) {
            for (auto const & member : element.members) {
                HgiShaderFunctionAddConstantParam(
                    funcDesc,
                    /*name=*/member.name,
                    /*type=*/_ConvertBoolType(member.dataType));
            }
        } else if (element.kind == Kind::BUFFER_READ_ONLY) {
            if (TF_VERIFY(element.members.size() == 1)) {
                auto const & member = element.members.front();
                HgiShaderFunctionAddBuffer(
                    funcDesc,
                    /*name=*/member.name,
                    /*type=*/_ConvertBoolType(member.dataType),
                    /*bindIndex=*/_GetLocation(element, metaData),
                    /*binding=*/HgiBindingTypePointer);
            }
        } else if (element.kind == Kind::BUFFER_READ_WRITE) {
            if (TF_VERIFY(element.members.size() == 1)) {
                auto const & member = element.members.front();
                HgiShaderFunctionAddWritableBuffer(
                    funcDesc,
                    /*name=*/member.name,
                    /*type=*/_ConvertBoolType(member.dataType),
                    /*bindIndex=*/_GetLocation(element, metaData));
            }
        }
    }
}

void
_ResourceGenerator::_GenerateHgiTextureResources(
    HgiShaderFunctionDesc *funcDesc,
    TfToken const & shaderStage,
    HioGlslfxResourceLayout::TextureElementVector const & textureElements,
    HdSt_ResourceBinder::MetaData const & metaData)
{
    using TextureType = HioGlslfxResourceLayout::TextureType;

    for (auto const & texture : textureElements) {
        HgiShaderTextureType const textureType =
            texture.textureType == TextureType::SHADOW_TEXTURE
                ? HgiShaderTextureTypeShadowTexture
                : texture.textureType == TextureType::ARRAY_TEXTURE
                    ? HgiShaderTextureTypeArrayTexture
                    : HgiShaderTextureTypeTexture;
        HdFormat const hdTextureFormat =
            HdStHioConversions::GetHdFormat(texture.format);
        if (texture.arraySize > 0) {
            HgiShaderFunctionAddArrayOfTextures(
                funcDesc,
                texture.name,
                texture.arraySize,
                texture.bindingIndex,
                texture.dim,
                HdStHgiConversions::GetHgiFormat(hdTextureFormat),
                textureType);
        } else {
            HgiShaderFunctionAddTexture(
                funcDesc,
                texture.name,
                texture.bindingIndex,
                texture.dim,
                HdStHgiConversions::GetHgiFormat(hdTextureFormat),
                textureType);
        }
    }
}

void
_ResourceGenerator::_GenerateGLSLResources(
    HgiShaderFunctionDesc *funcDesc,
    std::stringstream &str,
    TfToken const &shaderStage,
    HioGlslfxResourceLayout::ElementVector const & elements,
    HdSt_ResourceBinder::MetaData const & metaData)
{
    for (auto const & element : elements) {
        switch (element.kind) {
            case HioGlslfxResourceLayout::Kind::VALUE:
                switch (element.inOut) {
                    case HioGlslfxResourceLayout::InOut::STAGE_IN:
                        if (shaderStage == HdShaderTokens->vertexShader) {;
                            str << "layout (location = "
                                << _GetLocation(element, metaData) << ") ";
                        }
                        str << "in ";
                        break;
                    case HioGlslfxResourceLayout::InOut::STAGE_OUT:
                        if (shaderStage == HdShaderTokens->fragmentShader) {
                            str << "layout (location = "
                                << _GetLocation(element, metaData) << ") ";
                        }
                        str << "out ";
                        break;
                    case HioGlslfxResourceLayout::InOut::NONE:
                        break;
                    default:
                        break;
                }
                if (element.qualifiers == _tokens->flat) {
                    str << "flat ";
                } else if (element.qualifiers == _tokens->noperspective) {
                    str << "noperspective ";
                } else if (element.qualifiers == _tokens->centroid) {
                    str << "centroid ";
                } else if (element.qualifiers == _tokens->sample) {
                    str << "sample ";
                } else if (element.qualifiers == _tokens->patch) {
                    str << "patch ";
                }
                str << element.dataType << " "
                    << element.name;
                if (element.arraySize.IsEmpty()) {
                    str << ";\n";
                } else {
                    str << "[" << element.arraySize << "];\n";
                }
                break;
            case HioGlslfxResourceLayout::Kind::BLOCK:
                if (element.inOut == HioGlslfxResourceLayout::InOut::STAGE_IN) {
                    str << "in ";
                } else {
                    str << "out ";
                }
                str << element.aggregateName << " {\n";
                for (auto const & member : element.members) {
                    str << "    ";
                    if (member.qualifiers == _tokens->flat) {
                        str << "flat ";
                    }
                    else if (member.qualifiers == _tokens->noperspective) {
                        str << "noperspective ";
                    }
                    else if (member.qualifiers == _tokens->centroid) {
                        str << "centroid ";
                    }
                    else if (member.qualifiers == _tokens->sample) {
                        str << "sample ";
                    }
                    str << member.dataType << " " << member.name;
                    if (member.arraySize.IsEmpty()) {
                        str << ";\n";
                    } else {
                        str << "[" << member.arraySize << "];\n";
                    }
                }
                str << "} " << element.name;
                if (element.arraySize.IsEmpty()) {
                    str << ";\n";
                } else {
                    str << "[" << element.arraySize << "];\n";
                }
                break;
            case HioGlslfxResourceLayout::Kind::QUALIFIER:
                if (shaderStage == HdShaderTokens->tessControlShader) {
                    if (element.inOut == InOut::STAGE_OUT) {
                        funcDesc->tessellationDescriptor.numVertsPerPatchOut =
                            element.qualifiers.GetString();
                    }
                } else if (shaderStage == HdShaderTokens->tessEvalShader) {
                    if (element.inOut == InOut::STAGE_IN) {
                        if (element.qualifiers == _tokens->triangles) {
                            funcDesc->tessellationDescriptor.patchType =
                                HgiShaderFunctionTessellationDesc::
                                    PatchType::Triangles;
                        } else if (element.qualifiers == _tokens->quads) {
                            funcDesc->tessellationDescriptor.patchType =
                                HgiShaderFunctionTessellationDesc::
                                    PatchType::Quads;
                        } else if (element.qualifiers == _tokens->isolines) {
                            funcDesc->tessellationDescriptor.patchType =
                                HgiShaderFunctionTessellationDesc::
                                    PatchType::Isolines;
                        } else if (element.qualifiers ==
                                            _tokens->equal_spacing) {
                            funcDesc->tessellationDescriptor.spacing =
                                HgiShaderFunctionTessellationDesc::
                                    Spacing::Equal;
                        } else if (element.qualifiers ==
                                            _tokens->fractional_even_spacing) {
                            funcDesc->tessellationDescriptor.spacing =
                                HgiShaderFunctionTessellationDesc::
                                    Spacing::FractionalEven;
                        } else if (element.qualifiers ==
                                            _tokens->fractional_odd_spacing) {
                            funcDesc->tessellationDescriptor.spacing =
                                HgiShaderFunctionTessellationDesc::
                                    Spacing::FractionalOdd;
                        } else if (element.qualifiers == _tokens->cw) {
                            funcDesc->tessellationDescriptor.ordering =
                                HgiShaderFunctionTessellationDesc::
                                    Ordering::CW;
                        } else if (element.qualifiers == _tokens->ccw) {
                            funcDesc->tessellationDescriptor.ordering =
                                HgiShaderFunctionTessellationDesc::
                                    Ordering::CCW;
                        }
                    }
                } else if (shaderStage == HdShaderTokens->geometryShader) {
                    if (element.inOut == InOut::STAGE_IN) {
                        if (element.qualifiers == _tokens->points) {
                            funcDesc->geometryDescriptor.inPrimitiveType =
                                HgiShaderFunctionGeometryDesc::InPrimitiveType::
                                Points;
                        } else if (element.qualifiers == _tokens->lines) {
                            funcDesc->geometryDescriptor.inPrimitiveType =
                                HgiShaderFunctionGeometryDesc::InPrimitiveType::
                                Lines;
                        } else if (element.qualifiers ==
                            _tokens->lines_adjacency) {
                            funcDesc->geometryDescriptor.inPrimitiveType =
                                HgiShaderFunctionGeometryDesc::InPrimitiveType::
                                LinesAdjacency;
                        } else if (element.qualifiers == _tokens->triangles) {
                            funcDesc->geometryDescriptor.inPrimitiveType =
                                HgiShaderFunctionGeometryDesc::InPrimitiveType::
                                Triangles;
                        } else if (element.qualifiers ==
                            _tokens->triangles_adjacency) {
                            funcDesc->geometryDescriptor.inPrimitiveType =
                                HgiShaderFunctionGeometryDesc::InPrimitiveType::
                                TrianglesAdjacency;
                        }
                    } else if (element.inOut == InOut::STAGE_OUT) {
                        if (element.qualifiers == _tokens->points) {
                            funcDesc->geometryDescriptor.outPrimitiveType =
                                HgiShaderFunctionGeometryDesc::
                                OutPrimitiveType::Points;
                        } else if (element.qualifiers == _tokens->line_strip) {
                            funcDesc->geometryDescriptor.outPrimitiveType =
                                HgiShaderFunctionGeometryDesc::
                                OutPrimitiveType::LineStrip;
                        } else if (element.qualifiers ==
                            _tokens->triangle_strip) {
                            funcDesc->geometryDescriptor.outPrimitiveType =
                                HgiShaderFunctionGeometryDesc::
                                OutPrimitiveType::TriangleStrip;
                        } else {
                            // Assume any other GS stage out qualifier will be 
                            // the number of max vertices.
                            funcDesc->geometryDescriptor.outMaxVertices = 
                                element.qualifiers.GetString();
                        }
                    }
                } else if (element.qualifiers == _tokens->early_fragment_tests){
                    //   GLSL: "layout (early_fragment_tests) in;"
                    //   MSL: "[[early_fragment_tests]]"
                    funcDesc->fragmentDescriptor.earlyFragmentTests = true;
                }
                break;
            case HioGlslfxResourceLayout::Kind::UNIFORM_VALUE:
                str << "layout(location = " <<
                        _GetLocation(element, metaData)
                    << ") uniform "
                    << element.dataType << " *" << element.name;
                if (!element.arraySize.IsEmpty()) {
                    str << "[" << element.arraySize << "]";
                }
                str << ";\n";
                break;
            case HioGlslfxResourceLayout::Kind::UNIFORM_BLOCK:
                str << "layout(std140, binding = " <<
                        _GetLocation(element, metaData)
                    << ") uniform ubo_" << element.name
                    << " {\n"
                    << "    " << element.dataType << " "
                    << element.name;
                if (!element.arraySize.IsEmpty()) {
                    str << "[" << element.arraySize << "]";
                }
                str << ";\n};\n";
                break;
            case HioGlslfxResourceLayout::Kind::UNIFORM_BLOCK_CONSTANT_PARAMS:
                str << "layout(std140, binding = " <<
                        _GetLocation(element, metaData)
                    << ") uniform ubo_" << element.name
                    << " {\n";
                for (auto const & member : element.members) {
                    str << member.dataType << " " << member.name << ";\n";
                }
                str << "};\n";
                break;
            case HioGlslfxResourceLayout::Kind::BUFFER_READ_ONLY:
            case HioGlslfxResourceLayout::Kind::BUFFER_READ_WRITE:
                {
                auto const & member = element.members.front();
                uint32_t location = _GetLocation(element, metaData);
                str << "layout(std430, binding = " << location
                    << ") buffer ssbo_" << location
                    << " {\n"
                    << "    " << member.dataType << " "
                    << member.name << "[];\n"
                    << "};\n";
                }
                break;
            default:
                break;
        }
    }
}

static
std::string
_GetGLSLSamplerTypePrefix(HioFormat hioFormat)
{
    GLenum const hioType = HioGetHioType(hioFormat);
    switch (hioType) {
        case HioTypeUnsignedByte:
        case HioTypeUnsignedByteSRGB:
        case HioTypeUnsignedShort:
        case HioTypeUnsignedInt:
            return "u";
        case HioTypeSignedByte:
        case HioTypeSignedShort:
        case HioTypeInt:
            return "i";
        default:
            return "";
    }
}

void
_ResourceGenerator::_GenerateGLSLTextureResources(
    std::stringstream &str,
    TfToken const &shaderStage,
    HioGlslfxResourceLayout::TextureElementVector const & textureElements,
    HdSt_ResourceBinder::MetaData const & metaData)
{
    for (auto const & texture : textureElements) {
        bool const isArrayTexture =
            (texture.textureType == TextureType::ARRAY_TEXTURE);

        bool const isShadowTexture =
            (texture.textureType == TextureType::SHADOW_TEXTURE);

        bool const isArrayOfTexture = (texture.arraySize > 0);

        const std::string typePrefix =
            _GetGLSLSamplerTypePrefix(texture.format);

        std::string const samplerType =
            typePrefix +
            (isShadowTexture
                ? "sampler" + std::to_string(texture.dim) + "DShadow"
                : "sampler" + std::to_string(texture.dim) + "D") +
            (isArrayTexture ? "Array" : "");

        std::string const resultType =
            typePrefix +
            (isShadowTexture ? "float" : "vec4");

        std::string const resourceName =
            "sampler" + std::to_string(texture.dim) + "d_" +
            texture.name.GetString();

        int const coordDim =
            (isArrayTexture || isShadowTexture)
                ? (texture.dim + 1) : texture.dim;

        std::string const intCoordType = coordDim == 1 ?
            "int" :
            "ivec" + std::to_string(coordDim);
        std::string const floatCoordType = coordDim == 1 ?
            "float" :
            "vec" + std::to_string(coordDim);

        // Resource Declaration
        str << "layout (binding = "
            << std::to_string(texture.bindingIndex) << ") uniform "
            << samplerType << " " << resourceName;
        if (isArrayOfTexture) {
            str << "[" << std::to_string(texture.arraySize) << "];\n";
        } else {
            str << ";\n";
        }

        // Accessors
        if (isArrayOfTexture) {
            str << "#define HgiGetSampler_" << texture.name << "(index) "
                << resourceName << "[index]\n";

            str << resultType
                << " HgiGet_" << texture.name
                << "(int index, " << floatCoordType << " coord) {\n"
                << "  return texture(" << resourceName << "[index], coord)"
                << ";\n"
                << "}\n";

            str << resultType
                << " HgiTextureLod_" << texture.name
                << "(int index, " << floatCoordType << " coord, float lod) {\n"
                << "  return textureLod(" << resourceName
                << "[index], coord, lod)"
                << ";\n"
                << "}\n";

            if (!isShadowTexture) {
                str << resultType
                    << " HgiTexelFetch_" << texture.name
                    << "(int index, " << intCoordType << " coord) {\n"
                    << "  return texelFetch(" << resourceName
                    << "[index], coord, 0)"
                    << ";\n"
                    << "}\n";
            }
        } else {
            str << "#define HgiGetSampler_" << texture.name << "() "
                << resourceName << "\n";

            str << resultType
                << " HgiGet_" << texture.name
                << "(" << floatCoordType << " coord) {\n"
                << "  return texture(" << resourceName << ", coord)"
                << ";\n"
                << "}\n";

            str << resultType
                << " HgiTextureLod_" << texture.name
                << "(" << floatCoordType << " coord, float lod) {\n"
                << "  return textureLod(" << resourceName << ", coord, lod)"
                << ";\n"
                << "}\n";

            if (!isShadowTexture) {
                str << resultType
                    << " HgiTexelFetch_" << texture.name
                    << "(" << intCoordType << " coord) {\n"
                    << "  return texelFetch(" << resourceName << ", coord, 0)"
                    << ";\n"
                    << "}\n";
            }
        }
    }
}

static void
_AddVertexAttribElement(
    HioGlslfxResourceLayout::ElementVector *elements,
    TfToken const &name,
    TfToken const &dataType,
    int location,
    int arraySize = 0)
{
    elements->emplace_back(
        HioGlslfxResourceLayout::InOut::STAGE_IN,
        HioGlslfxResourceLayout::Kind::VALUE,
        dataType, name);
    if (location >= 0) {
        elements->back().location = static_cast<uint32_t>(location);
    }
}

static void
_AddInterstageElement(
    HioGlslfxResourceLayout::ElementVector *elements,
    HioGlslfxResourceLayout::InOut inOut,
    TfToken const &name,
    TfToken const &dataType,
    TfToken const &arraySize = TfToken(),
    TfToken const &qualifier = TfToken())
{
    elements->emplace_back(
        inOut,
        HioGlslfxResourceLayout::Kind::VALUE,
        dataType, name, arraySize, qualifier);
}

static void
_AddInterstageBlockElement(
    HioGlslfxResourceLayout::ElementVector *elements,
    HioGlslfxResourceLayout::InOut inOut,
    TfToken const &blockName,
    TfToken const &instanceName,
    HioGlslfxResourceLayout::MemberVector const &members,
    TfToken const &arraySize = TfToken())
{
    elements->emplace_back(
        inOut,
        HioGlslfxResourceLayout::Kind::BLOCK,
        HioGlslfxResourceLayoutTokens->block,
        instanceName,
        arraySize);
    elements->back().aggregateName = blockName;
    elements->back().members = members;
}

static void
_AddUniformValueElement(
    HioGlslfxResourceLayout::ElementVector *elements,
    TfToken const &name,
    TfToken const &dataType,
    int location,
    int arraySize = 0)
{
    TfToken const arraySizeArg =
        ((arraySize > 0) ? TfToken(std::to_string(arraySize)) : TfToken());

    elements->emplace_back(
        HioGlslfxResourceLayout::InOut::NONE,
        HioGlslfxResourceLayout::Kind::UNIFORM_VALUE,
        dataType, name, arraySizeArg);
    elements->back().members.emplace_back(dataType, name);
    if (location >= 0) {
        elements->back().location = static_cast<uint32_t>(location);
    }
}

static void
_AddUniformBufferElement(
    HioGlslfxResourceLayout::ElementVector *elements,
    TfToken const &name,
    TfToken const &dataType,
    int location,
    int arraySize = 0)
{
    TfToken const arraySizeArg =
        ((arraySize > 0) ? TfToken(std::to_string(arraySize)) : TfToken());

    elements->emplace_back(
        HioGlslfxResourceLayout::InOut::NONE,
        HioGlslfxResourceLayout::Kind::UNIFORM_BLOCK,
        dataType, name, arraySizeArg);
    elements->back().members.emplace_back(dataType, name);
    if (location >= 0) {
        elements->back().location = static_cast<uint32_t>(location);
    }
}

static void
_AddBufferElement(
    HioGlslfxResourceLayout::ElementVector *elements,
    TfToken const &name,
    TfToken const &dataType,
    int location,
    int arraySize = 0)
{
    elements->emplace_back(
        HioGlslfxResourceLayout::InOut::NONE,
        HioGlslfxResourceLayout::Kind::BUFFER_READ_ONLY,
        dataType, name);
    elements->back().members.emplace_back(dataType, name);
    if (location >= 0) {
        elements->back().location = static_cast<uint32_t>(location);
    }
}

static void
_AddWritableBufferElement(
    HioGlslfxResourceLayout::ElementVector *elements,
    TfToken const &name,
    TfToken const &dataType,
    int location)
{
    elements->emplace_back(
        HioGlslfxResourceLayout::InOut::NONE,
        HioGlslfxResourceLayout::Kind::BUFFER_READ_WRITE,
        dataType, name);
    elements->back().members.emplace_back(dataType, name);
    if (location >= 0) {
        elements->back().location = static_cast<uint32_t>(location);
    }
}

static void
_AddTextureElement(
    HioGlslfxResourceLayout::TextureElementVector *textureElements,
    TfToken const &name,
    int dim,
    int bindingIndex,
    HioFormat format = HioFormatFloat32Vec4,
    TextureType textureType = TextureType::TEXTURE)
{
    textureElements->emplace_back(
        name, dim, bindingIndex, format, textureType);
}

static void
_AddArrayOfTextureElement(
    HioGlslfxResourceLayout::TextureElementVector *textureElements,
    TfToken const &name,
    int dim,
    int bindingIndex,
    HioFormat format = HioFormatFloat32Vec4,
    TextureType textureType = TextureType::TEXTURE,
    int arraySize = 0)
{
    textureElements->emplace_back(
        name, dim, bindingIndex, format, textureType, arraySize);
}

static bool
_IsAtomicBufferShaderResource(
    HioGlslfxResourceLayout::ElementVector &elements,
    TfToken const &name)
{
    for (auto const &element : elements) {
        if (element.name == name &&
            element.kind == HioGlslfxResourceLayout::Kind::BUFFER_READ_WRITE) {
            if (TF_VERIFY(element.members.size() == 1)) {
                TfToken const &dataType = element.members.front().dataType;
                if (dataType == _tokens->_atomic_int ||
                    dataType == _tokens->_atomic_uint) {
                    return true;
                }
            }
        }
    }
    return false;
}

} // anonymous namespace

void
HdSt_CodeGen::_GetShaderResourceLayouts(
    HdStShaderCodeSharedPtrVector const & shaders)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TfTokenVector const shaderStages = {
        HdShaderTokens->vertexShader,
        HdShaderTokens->tessControlShader,
        HdShaderTokens->tessEvalShader,
        HdShaderTokens->geometryShader,
        HdShaderTokens->fragmentShader,
        HdShaderTokens->postTessControlShader,
        HdShaderTokens->postTessVertexShader,
        HdShaderTokens->computeShader,
    };

    for (auto const &shader : shaders) {
        VtDictionary layoutDict = shader->GetLayout(shaderStages);

        HioGlslfxResourceLayout::ParseLayout(
                &_resVS, HdShaderTokens->vertexShader, layoutDict);

        HioGlslfxResourceLayout::ParseLayout(
                &_resTCS, HdShaderTokens->tessControlShader, layoutDict);

        HioGlslfxResourceLayout::ParseLayout(
                &_resTES, HdShaderTokens->tessEvalShader, layoutDict);

        HioGlslfxResourceLayout::ParseLayout(
                &_resGS, HdShaderTokens->geometryShader, layoutDict);

        HioGlslfxResourceLayout::ParseLayout(
                &_resFS, HdShaderTokens->fragmentShader, layoutDict);

        HioGlslfxResourceLayout::ParseLayout(
                &_resPTCS, HdShaderTokens->postTessControlShader, layoutDict);

        HioGlslfxResourceLayout::ParseLayout(
                &_resPTVS, HdShaderTokens->postTessVertexShader, layoutDict);

        HioGlslfxResourceLayout::ParseLayout(
                &_resCS, HdShaderTokens->computeShader, layoutDict);
    }
}

void
HdSt_CodeGen::_PlumbInterstageElements(
    TfToken const &name,
    TfToken const &dataType)
{
    // Add resource elements to plumb interstage elements, e.g.
    // drawingCoord and interpolated primvar through active stages.

    std::string const &baseName = name.GetString();
    TfToken const  vs_outName( "vs_" + baseName);
    TfToken const tcs_outName("tcs_" + baseName);
    TfToken const tes_outName("tes_" + baseName);
    TfToken const  gs_outName( "gs_" + baseName);

    // Empty token for variables with no array size
    TfToken const noArraySize;

    // Interstage variables of type "int" require "flat" interpolation
    TfToken const &qualifier =
        (dataType == _tokens->_int) ? _tokens->flat : _tokens->_default;

    // Vertex attrib input for VS, PTCS, PTVS
    _resAttrib.emplace_back(InOut::STAGE_OUT, Kind::VALUE, dataType,
        vs_outName, noArraySize, qualifier);

    if (_hasTCS) {
        _resTCS.emplace_back(InOut::STAGE_IN, Kind::VALUE, dataType,
                vs_outName, _tokens->gl_MaxPatchVertices, qualifier);
        _resTCS.emplace_back(InOut::STAGE_OUT, Kind::VALUE, dataType,
                tcs_outName, _tokens->HD_NUM_PATCH_EVAL_VERTS, qualifier);
    }

    if (_hasTES) {
        _resTES.emplace_back(InOut::STAGE_IN, Kind::VALUE, dataType,
                tcs_outName, _tokens->gl_MaxPatchVertices, qualifier);
        _resTES.emplace_back(InOut::STAGE_OUT, Kind::VALUE, dataType,
                tes_outName, noArraySize, qualifier);
    }

    // Geometry shader inputs come from previous active stage
    if (_hasGS && _hasTES) {
        _resGS.emplace_back(InOut::STAGE_IN, Kind::VALUE, dataType,
                tes_outName, _tokens->HD_NUM_PRIMITIVE_VERTS, qualifier);
        _resGS.emplace_back(InOut::STAGE_OUT, Kind::VALUE, dataType,
                gs_outName, noArraySize, qualifier);
    } else if (_hasGS) {
        _resGS.emplace_back(InOut::STAGE_IN, Kind::VALUE, dataType,
                vs_outName, _tokens->HD_NUM_PRIMITIVE_VERTS, qualifier);
        _resGS.emplace_back(InOut::STAGE_OUT, Kind::VALUE, dataType,
                gs_outName, noArraySize, qualifier);
    }

    // Fragment shader inputs come from previous active stage
    if (_hasGS) {
        _resFS.emplace_back(InOut::STAGE_IN, Kind::VALUE, dataType,
                gs_outName, noArraySize, qualifier);
    } else if (_hasTES) {
        _resFS.emplace_back(InOut::STAGE_IN, Kind::VALUE, dataType,
                tes_outName, noArraySize, qualifier);
    } else {
        _resFS.emplace_back(InOut::STAGE_IN, Kind::VALUE, dataType,
                vs_outName, noArraySize, qualifier);
    }
}

static
std::string
_GetOSDCommonShaderSource()
{
    // Prepare OpenSubdiv common shader source for use in the shader
    // code declarations section and define some accessor methods and
    // forward declarations needed by the OpenSubdiv shaders.
    std::stringstream ss;

#if OPENSUBDIV_VERSION_NUMBER >= 30600
#if defined(__APPLE__)
    ss << OpenSubdiv::Osd::MTLPatchShaderSource::GetPatchDrawingShaderSource();
#else
    ss << "FORWARD_DECL(MAT4 GetProjectionMatrix());\n"
          "FORWARD_DECL(float GetTessLevel());\n"
          "mat4 OsdModelViewMatrix() { return mat4(1); }\n"
          "mat4 OsdProjectionMatrix() { return mat4(GetProjectionMatrix()); }\n"
          "float OsdTessLevel() { return GetTessLevel(); }\n"
          "\n";

    ss << OpenSubdiv::Osd::GLSLPatchShaderSource::GetPatchDrawingShaderSource();
#endif

#else // OPENSUBDIV_VERSION_NUMBER
    // Additional declarations are needed for older OpenSubdiv versions.

#if defined(__APPLE__)
    ss << "#define CONTROL_INDICES_BUFFER_INDEX 0\n"
       << "#define OSD_PATCHPARAM_BUFFER_INDEX 0\n"
       << "#define OSD_PERPATCHVERTEX_BUFFER_INDEX 0\n"
       << "#define OSD_PERPATCHTESSFACTORS_BUFFER_INDEX 0\n"
       << "#define OSD_KERNELLIMIT_BUFFER_INDEX 0\n"
       << "#define OSD_PATCHPARAM_BUFFER_INDEX 0\n"
       << "#define VERTEX_BUFFER_INDEX 0\n"

       // The ifdef for this in OSD is AFTER the first usage.
       << "#define OSD_MAX_VALENCE 4\n"

       << "\n"
       << "struct OsdInputVertexType {\n"
       << "    vec3 position;\n"
       << "};\n"
       << "\n";

    ss << OpenSubdiv::Osd::MTLPatchShaderSource::GetCommonShaderSource();
#else
    ss << "FORWARD_DECL(MAT4 GetProjectionMatrix());\n"
       << "FORWARD_DECL(float GetTessLevel());\n"
       << "mat4 OsdModelViewMatrix() { return mat4(1); }\n"
       << "mat4 OsdProjectionMatrix() { return mat4(GetProjectionMatrix()); }\n"
       << "int OsdPrimitiveIdBase() { return 0; }\n"
       << "float OsdTessLevel() { return GetTessLevel(); }\n"
       << "\n";

    ss << OpenSubdiv::Osd::GLSLPatchShaderSource::GetCommonShaderSource();
#endif
#endif // OPENSUBDIV_VERSION_NUMBER

    return ss.str();
}

static
std::string
_GetOSDPatchBasisShaderSource()
{
    std::stringstream ss;
#if defined(__APPLE__)
    ss << "#define OSD_PATCH_BASIS_METAL\n";
    ss << OpenSubdiv::Osd::MTLPatchShaderSource::GetPatchBasisShaderSource();
#else
    ss << "#define OSD_PATCH_BASIS_GLSL\n";
    ss << OpenSubdiv::Osd::GLSLPatchShaderSource::GetPatchBasisShaderSource();
#endif
    return ss.str();
}

HdStGLSLProgramSharedPtr
HdSt_CodeGen::Compile(HdStResourceRegistry*const registry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(_metaData,
                   "Metadata not properly initialized by resource binder.")) {
        return {}; 
    }
    
    _GetShaderResourceLayouts({_geometricShader});
    _GetShaderResourceLayouts(_shaders);

    // Capabilities.
    const HgiCapabilities *capabilities = registry->GetHgi()->GetCapabilities();
    const bool bindlessTextureEnabled =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsBindlessTextures);
    const bool bindlessBuffersEnabled = 
        capabilities->IsSet(HgiDeviceCapabilitiesBitsBindlessBuffers);
    const bool shaderDrawParametersEnabled =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsShaderDrawParameters);
    const bool builtinBarycentricsEnabled =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsBuiltinBarycentrics);
    const bool metalTessellationEnabled =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsMetalTessellation);
    const bool requiresBasePrimitiveOffset =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsBasePrimitiveOffset);
    const bool requiresPrimitiveIdEmulation =
         capabilities->IsSet(HgiDeviceCapabilitiesBitsPrimitiveIdEmulation);
    const bool doublePrecisionEnabled =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsShaderDoublePrecision);
    const bool minusOneToOneDepth =
        capabilities->IsSet(HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne);

    bool const useHgiResourceGeneration =
        IsEnabledHgiResourceGeneration(registry->GetHgi());

    // shader sources
    // geometric shader owns main()
    std::string vertexShader =
        _geometricShader->GetSource(HdShaderTokens->vertexShader);
    std::string tessControlShader =
        _geometricShader->GetSource(HdShaderTokens->tessControlShader);
    std::string tessEvalShader =
        _geometricShader->GetSource(HdShaderTokens->tessEvalShader);
    std::string postTessControlShader =
        _geometricShader->GetSource(HdShaderTokens->postTessControlShader);
    std::string postTessVertexShader =
        _geometricShader->GetSource(HdShaderTokens->postTessVertexShader);
    std::string geometryShader =
        _geometricShader->GetSource(HdShaderTokens->geometryShader);
    std::string fragmentShader =
        _geometricShader->GetSource(HdShaderTokens->fragmentShader);
    std::string computeShader =
        _geometricShader->GetSource(HdShaderTokens->computeShader);

    _hasVS  = (!vertexShader.empty());
    _hasTCS = (!tessControlShader.empty());
    _hasTES = (!tessEvalShader.empty());
    _hasPTCS = (!postTessControlShader.empty()) && metalTessellationEnabled;
    _hasPTVS = (!postTessVertexShader.empty()) && metalTessellationEnabled;
    _hasGS  = (!geometryShader.empty()) && !metalTessellationEnabled;
    _hasFS  = (!fragmentShader.empty());
    _hasCS  = (!computeShader.empty());

    // Initialize source buckets
    _genDefines.str(""); _genDecl.str(""); _genAccessors.str("");
    _genVS.str(""); _genTCS.str(""); _genTES.str("");
    _genPTCS.str(""); _genPTVS.str("");
    _genGS.str(""); _genFS.str(""); _genCS.str("");
    _procVS.str(""); _procTCS.str(""); _procTES.str(""); _procGS.str("");
    _procPTVSOut.str("");

    _genDefines << "\n// //////// Codegen Defines //////// \n";
    _genDecl << "\n// //////// Codegen Decl //////// \n";
    _genAccessors << "\n// //////// Codegen Accessors //////// \n";
    _genVS << "\n// //////// Codegen VS Source //////// \n";
    _genTCS << "\n// //////// Codegen TCS Source //////// \n";
    _genTES << "\n// //////// Codegen TES Source //////// \n";
    _genPTCS << "\n// //////// Codegen PTCS Source //////// \n";
    _genPTVS << "\n// //////// Codegen PTVS Source //////// \n";
    _genGS << "\n// //////// Codegen GS Source //////// \n";
    _genFS << "\n// //////// Codegen FS Source //////// \n";
    _genCS << "\n// //////// Codegen CS Source //////// \n";
    _procVS << "\n// //////// Codegen Proc VS //////// \n";
    _procTCS << "\n// //////// Codegen Proc TCS //////// \n";
    _procTES << "\n// //////// Codegen Proc TES //////// \n";
    _procGS << "\n// //////// Codegen Proc GS //////// \n";

    // Used in glslfx files to determine if it is using new/old
    // imaging system. It can also be used as API guards when
    // we need new versions of Storm shading. 
    _genDefines << "#define HD_SHADER_API " << HD_SHADER_API << "\n";

    // XXX: this macro is still used in GlobalUniform.
    _genDefines << "#define MAT4 " <<
        HdStGLConversions::GetGLSLTypename(
            HdVtBufferSource::GetDefaultMatrixType()) << "\n";

    // a trick to tightly pack unaligned data (vec3, etc) into SSBO/UBO.
    _genDefines << _GetPackedTypeDefinitions();

    if (_materialTag == HdStMaterialTagTokens->masked) {
        _genFS << "#define HD_MATERIAL_TAG_MASKED 1\n";
    }
    if (doublePrecisionEnabled) {
        _genFS << "#define HD_SHADER_SUPPORTS_DOUBLE_PRECISION\n";
    }
    if (minusOneToOneDepth) {
        _genFS << "#define HD_MINUS_ONE_TO_ONE_DEPTH_RANGE\n";
    }
    if (bindlessBuffersEnabled) {
        _genVS << "#define HD_BINDLESS_BUFFERS_ENABLED\n";
    }

    // ------------------
    // Custom Buffer Bindings
    // ----------------------
    // For custom buffer bindings, more code can be generated; a full spec is
    // emitted based on the binding declaration.
    TF_FOR_ALL(binDecl, _metaData->customBindings) {
        _genDefines << "#define "
                    << binDecl->name << "_Binding " 
                    << binDecl->binding.GetLocation() << "\n";
        _genDefines << "#define HD_HAS_" << binDecl->name << " 1\n";

        // typeless binding doesn't need declaration nor accessor.
        if (binDecl->dataType.IsEmpty()) continue;

        // atomics can't be trivially passed by value, and Storm does not
        // access them via HdGet_ accessors. Skip generation.
        if (_IsAtomicBufferShaderResource(_resFS, binDecl->name)) {
            continue;
        }

        _EmitDeclaration(&_resCommon,
                         binDecl->name,
                         binDecl->dataType,
                         binDecl->binding,
                         binDecl->isWritable);

        _EmitAccessor(_genAccessors,
                      binDecl->name,
                      binDecl->dataType,
                      binDecl->binding,
                      (binDecl->binding.GetType() == HdStBinding::UNIFORM)
                      ? NULL : "localIndex");
    }

    TF_FOR_ALL(it, _metaData->customInterleavedBindings) {
        // note: _constantData has been sorted by offset in HdSt_ResourceBinder.
        // XXX: not robust enough, should consider padding and layouting rules
        // to match with the logic in HdInterleavedMemoryManager if we
        // want to use a layouting policy other than default padding.

        HdStBinding binding = it->first;
        TfToken typeName(TfStringPrintf("CustomBlockData%d", binding.GetValue()));
        TfToken varName = it->second.blockName;

        _genDecl << "struct " << typeName << " {\n";
        // dbIt is StructEntry { name, dataType, offset, numElements }
        TF_FOR_ALL (dbIt, it->second.entries) {
            _genDefines << "#define HD_HAS_" << dbIt->name << " 1\n";
            _genDecl << "  " << _GetPackedType(_ConvertBoolType(dbIt->dataType), false)
                     << " " << dbIt->name;

            if (dbIt->arraySize > 1) {
                _genDefines << "#define HD_NUM_" << dbIt->name
                            << " " << dbIt->arraySize << "\n";
                _genDecl << "[" << dbIt->arraySize << "]";
            }
            _genDecl <<  ";\n";

            if (it->second.arraySize > 0) {
                _EmitStructAccessor(_genAccessors, varName, 
                                    dbIt->name, dbIt->dataType, dbIt->arraySize,
                                    "localIndex", dbIt->concatenateNames);
            } else {
                _EmitStructAccessor(_genAccessors, varName, 
                                    dbIt->name, dbIt->dataType, dbIt->arraySize,
                                    NULL,  dbIt->concatenateNames);
            }

            if (dbIt->name == HdShaderTokens->clipPlanes) {
                _hasClipPlanes = true;
            }
        }

        _genDecl << "};\n";
        _EmitDeclaration(&_resCommon, varName, typeName, binding,
                         /*isWritable=*/false, it->second.arraySize);
    }

    // HD_NUM_PATCH_VERTS, HD_NUM_PRIMTIIVE_VERTS
    if (_geometricShader->IsPrimTypePatches()) {
        _genDefines << "#define HD_NUM_PATCH_VERTS "
                    << _geometricShader->GetPrimitiveIndexSize() << "\n";
        _genDefines << "#define HD_NUM_PATCH_EVAL_VERTS "
                    << _geometricShader->GetNumPatchEvalVerts() << "\n";
    }
    _genDefines << "#define HD_NUM_PRIMITIVE_VERTS "
                << _geometricShader->GetNumPrimitiveVertsForGeometryShader()
                << "\n";

    // include ptex utility (if needed)
    TF_FOR_ALL (it, _metaData->shaderParameterBinding) {
        HdStBinding::Type bindingType = it->first.GetType();
        if (bindingType == HdStBinding::TEXTURE_PTEX_TEXEL ||
            bindingType == HdStBinding::BINDLESS_TEXTURE_PTEX_TEXEL) {
            _genDecl << _GetPtexTextureShaderSource();
            break;
        }
    }

    TF_FOR_ALL (it, _metaData->topologyVisibilityData) {
        TF_FOR_ALL (pIt, it->second.entries) {
            _genDefines << "#define HD_HAS_" << pIt->name  << " 1\n";
        }
    }

    // primvar existence macros

    // XXX: this is temporary, until we implement the fallback value definition
    // for any primvars used in glslfx.
    // Note that this #define has to be considered in the hash computation
    // since it changes the source code. However we have already combined the
    // entries of instanceData into the hash value, so it's not needed to be
    // added separately, at least in current usage.
    TF_FOR_ALL (it, _metaData->constantData) {
        TF_FOR_ALL (pIt, it->second.entries) {
            _genDefines << "#define HD_HAS_" << pIt->name << " 1\n";
        }
    }
    TF_FOR_ALL (it, _metaData->instanceData) {
        _genDefines << "#define HD_HAS_INSTANCE_" << it->second.name << " 1\n";
        _genDefines << "#define HD_HAS_"
                    << it->second.name << "_" << it->second.level << " 1\n";
    }
    _genDefines << "#define HD_INSTANCER_NUM_LEVELS "
                << _metaData->instancerNumLevels << "\n"
                << "#define HD_INSTANCE_INDEX_WIDTH "
                << (_metaData->instancerNumLevels+1) << "\n";
    if (!_geometricShader->IsPrimTypePoints()) {
        TF_FOR_ALL (it, _metaData->elementData) {
            _genDefines << "#define HD_HAS_" << it->second.name << " 1\n";
        }
        TF_FOR_ALL (it, _metaData->fvarData) {
            _genDefines << "#define HD_HAS_" << it->second.name << " 1\n";
        }
    }
    TF_FOR_ALL (it, _metaData->vertexData) {
        _genDefines << "#define HD_HAS_" << it->second.name << " 1\n";
    }
    TF_FOR_ALL (it, _metaData->varyingData) {
        _genDefines << "#define HD_HAS_" << it->second.name << " 1\n";
    }
    TF_FOR_ALL (it, _metaData->shaderParameterBinding) {
        // XXX: HdStBinding::PRIMVAR_REDIRECT won't define an accessor if it's
        // an alias of like-to-like, so we want to suppress the HD_HAS_* flag
        // as well.

        // For PRIMVAR_REDIRECT, the HD_HAS_* flag will be defined after
        // the corresponding HdGet_* function.

        // XXX: (HYD-1882) The #define HD_HAS_... for a primvar
        // redirect will be defined immediately after the primvar
        // redirect HdGet_... in the loop over
        // _metaData->shaderParameterBinding below.  Given that this
        // loop is not running in a canonical order (e.g., textures
        // first, then primvar redirects, ...) and that the texture is
        // picking up the HD_HAS_... flag, the answer to the following
        // question is random:
        //
        // If there is a texture trying to use a primvar called NAME
        // for coordinates and there is a primvar redirect called NAME,
        // will the texture use it or not?
        // 
        HdStBinding::Type bindingType = it->first.GetType();
        if (bindingType != HdStBinding::PRIMVAR_REDIRECT) {
            _genDefines << "#define HD_HAS_" << it->second.name << " 1\n";
        }

        // For any texture shader parameter we also emit the texture 
        // coordinates associated with it
        if (bindingType == HdStBinding::TEXTURE_2D ||
            bindingType == HdStBinding::BINDLESS_TEXTURE_2D ||
            bindingType == HdStBinding::ARRAY_OF_TEXTURE_2D ||
            bindingType == HdStBinding::BINDLESS_ARRAY_OF_TEXTURE_2D ||
            bindingType == HdStBinding::TEXTURE_UDIM_ARRAY || 
            bindingType == HdStBinding::BINDLESS_TEXTURE_UDIM_ARRAY) {
            _genDefines
                << "#define HD_HAS_COORD_" << it->second.name << " 1\n";
        }
    }

    // Needed for patch-based position and primvar refinement
    if (_geometricShader->IsPrimTypeMesh() &&
        _geometricShader->IsPrimTypePatches()) {
        if (_hasPTCS) {
            _genPTCS << _GetOSDPatchBasisShaderSource();
        }
        if (_hasPTVS) {
            _genPTVS << _GetOSDPatchBasisShaderSource();
        }
    }

    // Needed for patch-based face-varying primvar refinement
    if (_geometricShader->GetFvarPatchType() == 
        HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE ||
        _geometricShader->GetFvarPatchType() == 
        HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
        if (_hasGS) {
            _genGS << _GetOSDPatchBasisShaderSource();
        } else {
            _genFS << _GetOSDPatchBasisShaderSource();
        }
    }

    // Barycentric coordinates
    if (builtinBarycentricsEnabled) {
        _genFS << "vec3 GetBarycentricCoord() {\n"
                  "  return hd_BaryCoordNoPersp;\n"
                  "}\n";
    } else {
        if (_hasGS) {
            _AddInterstageElement(&_resGS,
                HioGlslfxResourceLayout::InOut::STAGE_OUT,
                /*name=*/_tokens->hd_barycentricCoord,
                /*dataType=*/_tokens->vec3,
                TfToken(),
                TfToken("noperspective"));
            _AddInterstageElement(&_resFS,
                HioGlslfxResourceLayout::InOut::STAGE_IN,
                /*name=*/_tokens->hd_barycentricCoord,
                /*dataType=*/_tokens->vec3,
                TfToken(),
                TfToken("noperspective"));

            _genFS << "vec3 GetBarycentricCoord() {\n"
                      "  return hd_barycentricCoord;\n"
                      "}\n";
        } else {
            _genFS << "vec3 GetBarycentricCoord() {\n"
                      "  return vec3(0);\n"
                      "}\n";
        }
    }

    // We plumb the evaluated position in patch from PTVS to FS since this
    // is more consistent than using built-in barycentric coords and can be
    // used even when builtin barycentric coords are not available. We pass
    // only the first two components between stages and provide an accessor
    // which can reconstruct the full three component barycentric form.
    if (_hasPTVS) {
        _AddInterstageElement(&_resPTVS,
            HioGlslfxResourceLayout::InOut::STAGE_OUT,
            /*name=*/_tokens->hd_tessCoord,
            /*dataType=*/_tokens->vec2);
        _AddInterstageElement(&_resFS,
            HioGlslfxResourceLayout::InOut::STAGE_IN,
            /*name=*/_tokens->hd_tessCoord,
            /*dataType=*/_tokens->vec2);

        _genFS << "vec2 GetTessCoord() {\n"
                  "  return hd_tessCoord;\n"
                  "}\n"
                  "vec3 GetTessCoordTriangle() {\n"
                  "  return vec3("
                  "hd_tessCoord.x, hd_tessCoord.y, "
                  "1 - hd_tessCoord.x - hd_tessCoord.y);\n"
                  "}\n";
    }

    // PrimitiveID emulation
    if (requiresPrimitiveIdEmulation) {
        if (_hasPTVS) {
            _AddInterstageElement(&_resPTVS,
                HioGlslfxResourceLayout::InOut::STAGE_OUT,
                /*name=*/_tokens->hd_patchID,
                /*dataType=*/_tokens->_uint);
            _AddInterstageElement(&_resFS,
                HioGlslfxResourceLayout::InOut::STAGE_IN,
                /*name=*/_tokens->hd_patchID,
                /*dataType=*/_tokens->_uint);
        }
    }

    // prep interstage plumbing function
    _procVS  << "void ProcessPrimvarsIn() {\n";

    _procTCS << "void ProcessPrimvarsOut() {\n";
    _procTES << "float InterpolatePrimvar("
                "float inPv0, float inPv1, float inPv2, float inPv3, "
                "vec4 basis, vec2 uv);\n"

                "vec2 InterpolatePrimvar("
                "vec2 inPv0, vec2 inPv1, vec2 inPv2, vec2 inPv3, "
                "vec4 basis, vec2 uv);\n"

                "vec3 InterpolatePrimvar("
                "vec3 inPv0, vec3 inPv1, vec3 inPv2, vec3 inPv3, "
                "vec4 basis, vec2 uv);\n"

                "vec4 InterpolatePrimvar("
                "vec4 inPv0, vec4 inPv1, vec4 inPv3, vec4 inPv3, "
                "vec4 basis, vec2 uv);\n"

                "void ProcessPrimvarsOut("
                "vec4 basis, int i0, int i1, int i2, int i3, vec2 uv) {\n";

    _procPTVSOut << "template <typename T>\n"
                    "T InterpolatePrimvar("
                    "T inPv0, T inPv1, T inPv2, T inPv3, vec4 basis, "
                    "vec2 uv = vec2()) {\n"
                    "  return"
                    " inPv0 * basis[0] +"
                    " inPv1 * basis[1] +"
                    " inPv2 * basis[2] +"
                    " inPv3 * basis[3];\n"
                    "}\n"
                    "void ProcessPrimvarsOut("
                    "vec4 basis, int i0, int i1, int i2, int i3, "
                    "vec2 uv = vec2()) {\n";

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
            _procGS << "FORWARD_DECL(vec4 GetPatchCoord(int index));\n"
                    << "void ProcessPrimvarsOut(int index) {\n"
                    << "  vec2 localST = GetPatchCoord(index).xy;\n";
            break;
        }
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BSPLINE:
        case HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
        {
            _procGS << "void ProcessPrimvarsOut(int index, vec2 tessST) {\n"
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
                // Each quad is split into two triangles with indices (0,1,2)
                // and (2,3,0).
                _procGS << "  const vec3 coords[4] = vec3[](\n"
                        << "   vec3(1,0,0), vec3(0,1,0), "
                        << "vec3(0,0,1), vec3(0,1,0)\n"
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

    if (_hasPTVS) {
        _procPTVSOut << "  hd_tessCoord = gl_TessCoord.xy;\n";
    }

    if (requiresPrimitiveIdEmulation) {
        _procPTVSOut << "  hd_patchID = patch_id;\n";
    }

    // generate drawing coord and accessors
    _GenerateDrawingCoord(shaderDrawParametersEnabled,
                          requiresBasePrimitiveOffset,
                          requiresPrimitiveIdEmulation);

    // generate primvars
    _GenerateConstantPrimvar();
    _GenerateInstancePrimvar();
    _GenerateElementPrimvar();
    _GenerateVertexAndFaceVaryingPrimvar();

    _GenerateTopologyVisibilityParameters();

    //generate shader parameters (is going last since it has primvar redirects)
    _GenerateShaderParameters(bindlessTextureEnabled);

    // finalize buckets
    _procVS  << "}\n";
    _procGS  << "}\n";
    _procTCS << "}\n";
    _procTES << "}\n";
    _procPTVSOut << "}\n";

    // insert interstage primvar plumbing procs into genVS/TCS/TES/GS
    _genVS  << _procVS.str();
    _genTCS << _procTCS.str();
    _genTES << _procTES.str();
    _genPTVS << _procPTVSOut.str();
    _genGS  << _procGS.str();

    // other shaders (renderPass, lighting, surface) first
    TF_FOR_ALL(it, _shaders) {
        HdStShaderCodeSharedPtr const &shader = *it;
        if (_hasVS) {
            _genVS  << shader->GetSource(HdShaderTokens->vertexShader);
        }
        if (_hasTCS) {
            _genTCS << shader->GetSource(HdShaderTokens->tessControlShader);
        }
        if (_hasTES) {
            _genTES << shader->GetSource(HdShaderTokens->tessEvalShader);
        }
        if (_hasPTCS) {
            _genPTCS << shader->GetSource(
                HdShaderTokens->postTessControlShader);
        }
        if (_hasPTVS) {
            _genPTVS << shader->GetSource(HdShaderTokens->postTessVertexShader);
            _genPTVS << shader->GetSource(HdShaderTokens->displacementShader);
        }
        if (_hasGS) {
            _genGS  << shader->GetSource(HdShaderTokens->geometryShader);
            _genGS  << shader->GetSource(HdShaderTokens->displacementShader);
        }
        if (_hasFS) {
            _genFS  << shader->GetSource(HdShaderTokens->fragmentShader);
        }
    }
    
    // We need to include OpenSubdiv shader source only when processing
    // refined meshes. For all other meshes we need only a simplified
    // method of patch coord interpolation.
    if (_geometricShader->IsPrimTypeRefinedMesh()) {
        // Include OpenSubdiv shader source and use full patch interpolation.
        _osd << _GetOSDCommonShaderSource();
        _osd <<
            "vec4 InterpolatePatchCoord(vec2 uv, ivec3 patchParam)\n"
            "{\n"
            "    return OsdInterpolatePatchCoord(uv, patchParam);\n"
            "}\n"
            "vec4 InterpolatePatchCoordTriangle(vec2 uv, ivec3 patchParam)\n"
            "{\n"
            "    return OsdInterpolatePatchCoordTriangle(uv, patchParam);\n"
            "}\n";
    } else if (_geometricShader->IsPrimTypeMesh()) {
        // Use simplified patch interpolation since all mesh faces are level 0.
        _osd <<
            "vec4 InterpolatePatchCoord(vec2 uv, ivec3 patchParam)\n"
            "{\n"
            "    // add 0.5 to integer values for more robust interpolation\n"
            "    return vec4(uv.x, uv.y, 0, patchParam.x+0.5f);\n"
            "}\n"
            "vec4 InterpolatePatchCoordTriangle(vec2 uv, ivec3 patchParam)\n"
            "{\n"
            "    return InterpolatePatchCoord(uv, patchParam);\n"
            "}\n";
    }

    // geometric shader
    _genVS  << vertexShader;
    _genTCS << tessControlShader;
    _genTES << tessEvalShader;
    _genPTCS << postTessControlShader;
    _genPTVS << postTessVertexShader;
    _genGS  << geometryShader;
    _genFS  << fragmentShader;
    _genCS  << computeShader;

    // Sanity check that if you provide a control shader, you have also provided
    // an evaluation shader (and vice versa)
    if (_hasTCS ^ _hasTES) {
        TF_CODING_ERROR(
            "tessControlShader and tessEvalShader must be provided together.");
        _hasTCS = _hasTES = false;
    };

    if (useHgiResourceGeneration) {
        return _CompileWithGeneratedHgiResources(registry);
    } else {
        return _CompileWithGeneratedGLSLResources(registry);
    }
}

HdStGLSLProgramSharedPtr
HdSt_CodeGen::CompileComputeProgram(HdStResourceRegistry*const registry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();


    if (!TF_VERIFY(_metaData,
                   "Metadata not properly initialized by resource binder.")) {
        return {}; 
    }

    _GetShaderResourceLayouts(_shaders);

    // Initialize source buckets
    _genDefines.str(""); _genDecl.str(""); _genAccessors.str("");
    _genVS.str(""); _genTCS.str(""); _genTES.str("");
    _genGS.str(""); _genFS.str(""); _genCS.str("");
    _genPTCS.str(""); _genPTVS.str("");
    _procVS.str(""); _procTCS.str(""); _procTES.str(""); _procGS.str("");

    _genDefines << "\n// //////// Codegen Defines //////// \n";
    _genDecl << "\n// //////// Codegen Decl //////// \n";
    _genAccessors << "\n// //////// Codegen Accessors //////// \n";
    _genVS << "\n// //////// Codegen VS Source //////// \n";
    _genTCS << "\n// //////// Codegen TCS Source //////// \n";
    _genTES << "\n// //////// Codegen TES Source //////// \n";
    _genGS << "\n// //////// Codegen GS Source //////// \n";
    _genFS << "\n// //////// Codegen FS Source //////// \n";
    _genCS << "\n// //////// Codegen CS Source //////// \n";
    _procVS << "\n// //////// Codegen Proc VS //////// \n";
    _procTCS << "\n// //////// Codegen Proc TCS //////// \n";
    _procTES << "\n// //////// Codegen Proc TES //////// \n";
    _procGS << "\n// //////// Codegen Proc GS //////// \n";

    // Used in glslfx files to determine if it is using new/old
    // imaging system. It can also be used as API guards when
    // we need new versions of Storm shading. 
    _genDefines << "#define HD_SHADER_API " << HD_SHADER_API << "\n";

    // a trick to tightly pack unaligned data (vec3, etc) into SSBO/UBO.
    _genDefines << _GetPackedTypeDefinitions();

    _hasCS = true;

    return _CompileWithGeneratedHgiResources(registry);
}

void
HdSt_CodeGen::_GenerateComputeParameters(HgiShaderFunctionDesc * const csDesc)
{
    std::stringstream accessors;

    bool const hasComputeData =
        !_metaData->computeReadWriteData.empty() ||
        !_metaData->computeReadOnlyData.empty();
    if (hasComputeData) {
        HgiShaderFunctionAddConstantParam(
            csDesc, "vertexOffset", _tokens->_int);
    }

    accessors << "// Read-Write Accessors & Mutators\n";
    TF_FOR_ALL(it, _metaData->computeReadWriteData) {
        TfToken const &name = it->second.name;
        HdStBinding const &binding = it->first;
        TfToken const &dataType = it->second.dataType;

        // For now, SSBO bindings use a flat type encoding.
        TfToken declDataType =
            (binding.GetType() == HdStBinding::SSBO
                ? _GetFlatType(dataType) : dataType);
        
        HgiShaderFunctionAddConstantParam(
            csDesc, name.GetString() + "Offset", _tokens->_int);
        HgiShaderFunctionAddConstantParam(
            csDesc, name.GetString() + "Stride", _tokens->_int);
        
        _genDefines << "#define HD_HAS_" << name << " 1\n";

        _EmitDeclaration(&_resCommon,
                name,
                declDataType,
                binding,
                /*isWritable=*/true);

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
    TF_FOR_ALL(it, _metaData->computeReadOnlyData) {
        TfToken const &name = it->second.name;
        HdStBinding const &binding = it->first;
        TfToken const &dataType = it->second.dataType;
        
        // For now, SSBO bindings use a flat type encoding.
        TfToken declDataType =
            (binding.GetType() == HdStBinding::SSBO
                ? _GetFlatType(dataType) : dataType);

        HgiShaderFunctionAddConstantParam(
            csDesc, name.GetString() + "Offset", _tokens->_int);
        HgiShaderFunctionAddConstantParam(
            csDesc, name.GetString() + "Stride", _tokens->_int);

        _genDefines << "#define HD_HAS_" << name << " 1\n";

        _EmitDeclaration(&_resCommon,
                name,
                declDataType,
                binding);
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

    _genAccessors << accessors.str();
    
    // other shaders (renderpass, lighting, surface) first
    TF_FOR_ALL(it, _shaders) {
        HdStShaderCodeSharedPtr const &shader = *it;
        _genCS  << shader->GetSource(HdShaderTokens->computeShader);
    }

    // thread indexing id
    HgiShaderFunctionAddStageInput(
        csDesc, "hd_GlobalInvocationID", "uvec3",
        HgiShaderKeywordTokens->hdGlobalInvocationID);

    // main
    _genCS << "void main() {\n";
    _genCS << "  int computeCoordinate = int(hd_GlobalInvocationID.x);\n";
    _genCS << "  compute(computeCoordinate);\n";
    _genCS << "}\n";

}

HdStGLSLProgramSharedPtr
HdSt_CodeGen::_CompileWithGeneratedGLSLResources(
    HdStResourceRegistry * const registry)
{
    // Generator assigns attribute and binding locations
    _ResourceGenerator resourceGen;

    // Create additional resource elements needed by interstage elements
    for (auto const &element : _resInterstage) {
        _PlumbInterstageElements(element.name, element.dataType);
    }

    // create GLSL program.
    HdStGLSLProgramSharedPtr glslProgram =
        std::make_shared<HdStGLSLProgram>(HdTokens->drawingShader, registry);

    bool shaderCompiled = false;

    // compile shaders
    // note: _vsSource, _fsSource etc are used for diagnostics (see header)
    if (_hasVS) {
        HgiShaderFunctionDesc desc;
        std::stringstream resDecl;
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->vertexShader, _resAttrib, _GetMetaData());
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->vertexShader, _resCommon, _GetMetaData());
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->vertexShader, _resVS, _GetMetaData());

        std::string const declarations =
            _genDefines.str();
        std::string const source =
            _genDecl.str() + resDecl.str() +
            _genAccessors.str() + _genVS.str();

        desc.shaderStage = HgiShaderStageVertex;
        desc.shaderCodeDeclarations = declarations.c_str();
        desc.shaderCode = source.c_str();
        desc.generatedShaderCodeOut = &_vsSource;

        HgiShaderFunctionAddStageInput(
            &desc, "hd_VertexID", "uint",
            HgiShaderKeywordTokens->hdVertexID);
        HgiShaderFunctionAddStageInput(
            &desc, "hd_InstanceID", "uint",
            HgiShaderKeywordTokens->hdInstanceID);
        HgiShaderFunctionAddStageInput(
            &desc, "hd_BaseInstance", "uint",
            HgiShaderKeywordTokens->hdBaseInstance);
    
        if (_hasClipPlanes) {
            HgiShaderFunctionAddStageOutput(
                &desc, "gl_ClipDistance", "float",
                "clip_distance", /*arraySize*/"HD_NUM_clipPlanes");
        }

        if (!glslProgram->CompileShader(desc)) {
            return nullptr;
        }
        shaderCompiled = true;
    }
    if (_hasTCS) {
        HgiShaderFunctionDesc desc;
        std::stringstream resDecl;
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->tessControlShader, _resCommon, _GetMetaData());
        resourceGen._GenerateGLSLResources(&desc, resDecl, 
            HdShaderTokens->tessControlShader, _resTCS, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _osd.str();
        std::string const source =
            _genDecl.str() + resDecl.str() +
            _genAccessors.str() + _genTCS.str();

        desc.shaderStage = HgiShaderStageTessellationControl;
        desc.shaderCodeDeclarations = declarations.c_str();
        desc.shaderCode = source.c_str();
        desc.generatedShaderCodeOut = &_tcsSource;

        if (!glslProgram->CompileShader(desc)) {
            return nullptr;
        }
        shaderCompiled = true;
    }
    if (_hasTES) {
        HgiShaderFunctionDesc desc;
        std::stringstream resDecl;
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->tessEvalShader, _resCommon, _GetMetaData());
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->tessEvalShader, _resTES, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _osd.str();
        std::string const source =
            _genDecl.str() + resDecl.str() +
            _genAccessors.str() + _genTES.str();

        desc.shaderStage = HgiShaderStageTessellationEval;
        desc.shaderCodeDeclarations = declarations.c_str();
        desc.shaderCode = source.c_str();
        desc.generatedShaderCodeOut = &_tesSource;

        if (_hasClipPlanes) {
            HgiShaderFunctionAddStageOutput(
                &desc, "gl_ClipDistance", "float",
                "clip_distance", /*arraySize*/"HD_NUM_clipPlanes");
        }

        if (!glslProgram->CompileShader(desc)) {
            return nullptr;
        }
        shaderCompiled = true;
    }
    if (_hasGS) {
        HgiShaderFunctionDesc desc;
        std::stringstream resDecl;
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->geometryShader, _resCommon, _GetMetaData());
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->geometryShader, _resGS, _GetMetaData());

        // material in GS
        resourceGen._GenerateGLSLResources(&desc, resDecl, 
            HdShaderTokens->geometryShader, _resMaterial, _GetMetaData());
        resourceGen._GenerateGLSLTextureResources(resDecl, 
            HdShaderTokens->geometryShader, _resTextures, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _osd.str();
        std::string const source =
            _genDecl.str() + resDecl.str() +
            _genAccessors.str() + _genGS.str();

        desc.shaderStage = HgiShaderStageGeometry;
        desc.shaderCodeDeclarations = declarations.c_str();
        desc.shaderCode = source.c_str();
        desc.generatedShaderCodeOut = &_gsSource;

        if (_hasClipPlanes) {
            HgiShaderFunctionAddStageOutput(
                &desc, "gl_ClipDistance", "float",
                "clip_distance", /*arraySize*/"HD_NUM_clipPlanes");
        }

        if (!glslProgram->CompileShader(desc)) {
            return nullptr;
        }
        shaderCompiled = true;
    }
    if (_hasFS) {
        HgiShaderFunctionDesc desc;
        std::stringstream resDecl;
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->fragmentShader, _resCommon, _GetMetaData());
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->fragmentShader, _resFS, _GetMetaData());

        // material in FS
        resourceGen._GenerateGLSLResources(&desc, resDecl,
            HdShaderTokens->fragmentShader, _resMaterial, _GetMetaData());
        resourceGen._GenerateGLSLTextureResources(resDecl,
            HdShaderTokens->fragmentShader, _resTextures, _GetMetaData());

        std::string const source =
            _genDefines.str() + _genDecl.str() + resDecl.str() + _osd.str() +
            _genAccessors.str() + _genFS.str();

        desc.shaderStage = HgiShaderStageFragment;
        desc.shaderCode = source.c_str();
        desc.generatedShaderCodeOut = &_fsSource;

        const bool builtinBarycentricsEnabled =
            registry->GetHgi()->GetCapabilities()->IsSet(
                HgiDeviceCapabilitiesBitsBuiltinBarycentrics);
        if (builtinBarycentricsEnabled) {
            HgiShaderFunctionAddStageInput(
                &desc, "hd_BaryCoordNoPersp", "vec3",
                HgiShaderKeywordTokens->hdBaryCoordNoPersp);
        }

        if (!glslProgram->CompileShader(desc)) {
            return nullptr;
        }
        shaderCompiled = true;
    }

    if (!shaderCompiled) {
        return nullptr;
    }

    return glslProgram;
}

HdStGLSLProgramSharedPtr
HdSt_CodeGen::_CompileWithGeneratedHgiResources(
    HdStResourceRegistry * const registry)
{
    // Generator assigns attribute and binding locations
    _ResourceGenerator resourceGen;

    // Create additional resource elements needed by interstage elements.
    // For compute-only shaders, we don't have a HdSt_GeometricShader.
    for (auto const &element : _resInterstage) {
        _PlumbInterstageElements(element.name, element.dataType);
    }

    // create GLSL program.
    HdStGLSLProgramSharedPtr glslProgram =
        std::make_shared<HdStGLSLProgram>(HdTokens->drawingShader, registry);

    bool shaderCompiled = false;

    if (_hasVS) {
        HgiShaderFunctionDesc vsDesc;
        vsDesc.shaderStage = HgiShaderStageVertex;

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &vsDesc,
            HdShaderTokens->vertexShader, _resAttrib, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &vsDesc,
            HdShaderTokens->vertexShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &vsDesc,
            HdShaderTokens->vertexShader, _resVS, _GetMetaData());

        std::string const declarations = _genDefines.str() + _genDecl.str();
        std::string const source = _genAccessors.str() + _genVS.str();

        vsDesc.shaderCodeDeclarations = declarations.c_str();
        vsDesc.shaderCode = source.c_str();
        vsDesc.generatedShaderCodeOut = &_vsSource;

        // builtins

        HgiShaderFunctionAddStageInput(
            &vsDesc, "hd_VertexID", "uint",
            HgiShaderKeywordTokens->hdVertexID);
        HgiShaderFunctionAddStageInput(
            &vsDesc, "hd_InstanceID", "uint",
            HgiShaderKeywordTokens->hdInstanceID);
        HgiShaderFunctionAddStageInput(
            &vsDesc, "hd_BaseInstance", "uint",
            HgiShaderKeywordTokens->hdBaseInstance);
        HgiShaderFunctionAddStageInput(
            &vsDesc, "gl_BaseVertex", "uint",
            HgiShaderKeywordTokens->hdBaseVertex);

        if (!_geometricShader->IsFrustumCullingPass()) {
            HgiShaderFunctionAddStageOutput(
                &vsDesc, "gl_Position", "vec4", "position");
            
            // For Metal, only set the role for the point size
            // if the primitive is a point list.
            char const* pointRole =
                (_geometricShader->GetPrimitiveType() ==
                HdSt_GeometricShader::PrimitiveType::PRIM_POINTS)
                ? "point_size" : "";
            HgiShaderFunctionAddStageOutput(
                &vsDesc, "gl_PointSize", "float", pointRole);
        }

        if (_hasClipPlanes) {
            HgiShaderFunctionAddStageOutput(
                &vsDesc, "gl_ClipDistance", "float",
                "clip_distance", /*arraySize*/"HD_NUM_clipPlanes");
        }

        if (!glslProgram->CompileShader(vsDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }

    if (_hasTCS) {
        HgiShaderFunctionDesc tcsDesc;
        tcsDesc.shaderStage = HgiShaderStageTessellationControl;

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &tcsDesc,
            HdShaderTokens->tessControlShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &tcsDesc,
            HdShaderTokens->tessControlShader, _resTCS, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _genDecl.str() + _osd.str();
        std::string const source = _genAccessors.str() + _genTCS.str();

        tcsDesc.shaderCodeDeclarations = declarations.c_str();
        tcsDesc.shaderCode = source.c_str();
        tcsDesc.generatedShaderCodeOut = &_tcsSource;
        
        if (!glslProgram->CompileShader(tcsDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }

    if (_hasTES) {
        HgiShaderFunctionDesc tesDesc;
        tesDesc.shaderStage = HgiShaderStageTessellationEval;

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &tesDesc,
            HdShaderTokens->tessEvalShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &tesDesc,
            HdShaderTokens->tessEvalShader, _resTES, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _genDecl.str() + _osd.str();
        std::string const source = _genAccessors.str() + _genTES.str();

        tesDesc.shaderCodeDeclarations = declarations.c_str();
        tesDesc.shaderCode = source.c_str();
        tesDesc.generatedShaderCodeOut = &_tesSource;

        if (_hasClipPlanes) {
            HgiShaderFunctionAddStageOutput(
                &tesDesc, "gl_ClipDistance", "float",
                "clip_distance", /*arraySize*/"HD_NUM_clipPlanes");
        }

        if (!glslProgram->CompileShader(tesDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }

    if (_hasPTCS) {
        HgiShaderFunctionDesc ptcsDesc;
        ptcsDesc.shaderStage = HgiShaderStagePostTessellationControl;

        if (_metaData->tessFactorsBinding.binding.IsValid()) {
            HdStBinding binding = _metaData->tessFactorsBinding.binding;
            _EmitDeclaration(&_resPTCS,
                             _metaData->tessFactorsBinding.name,
                             _metaData->tessFactorsBinding.dataType,
                             binding,
                             true);
        }

        ptcsDesc.tessellationDescriptor.numVertsPerPatchIn =
              std::to_string(_geometricShader->GetPrimitiveIndexSize());
        ptcsDesc.tessellationDescriptor.numVertsPerPatchOut =
              std::to_string(_geometricShader->GetNumPatchEvalVerts());

        ptcsDesc.tessellationDescriptor.patchType =
            (_geometricShader->IsPrimTypeTriangles() ||
        _geometricShader->GetPrimitiveType() ==
            HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE) ?
            HgiShaderFunctionTessellationDesc::PatchType::Triangles :
            HgiShaderFunctionTessellationDesc::PatchType::Quads;
        if (_geometricShader->GetHgiPrimitiveType() ==
            HgiPrimitiveTypePointList) {
            ptcsDesc.tessellationDescriptor.patchType =
            HgiShaderFunctionTessellationDesc::PatchType::Isolines;
        }

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptcsDesc,
            HdShaderTokens->postTessControlShader, _resAttrib, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptcsDesc,
            HdShaderTokens->postTessControlShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptcsDesc,
            HdShaderTokens->postTessControlShader, _resPTCS, _GetMetaData());

        // material in PTCS
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptcsDesc,
            HdShaderTokens->postTessControlShader, _resMaterial, _GetMetaData());
        resourceGen._GenerateHgiTextureResources(&ptcsDesc,
            HdShaderTokens->postTessControlShader, _resTextures, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _genDecl.str() + _osd.str();
        std::string const source = _genAccessors.str() + _genPTCS.str();

        ptcsDesc.shaderCodeDeclarations = declarations.c_str();
        ptcsDesc.shaderCode = source.c_str();
        ptcsDesc.generatedShaderCodeOut = &_ptcsSource;

        // builtins

        HgiShaderFunctionAddStageInput(
            &ptcsDesc, "hd_BaseInstance", "uint",
            HgiShaderKeywordTokens->hdBaseInstance);
        HgiShaderFunctionAddStageInput(
            &ptcsDesc, "patch_id", "uint",
            HgiShaderKeywordTokens->hdPatchID);

        std::string tessCoordType =
            (_geometricShader->IsPrimTypeTriangles() ||
             _geometricShader->GetPrimitiveType() ==
               HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE)
            ? "vec3" : "vec2";

        HgiShaderFunctionAddStageInput(
            &ptcsDesc, "gl_TessCoord", tessCoordType,
            HgiShaderKeywordTokens->hdPositionInPatch);

        HgiShaderFunctionAddStageInput(
            &ptcsDesc, "hd_InstanceID", "uint",
            HgiShaderKeywordTokens->hdInstanceID);

        HgiShaderFunctionAddStageOutput(
            &ptcsDesc, "gl_Position", "vec4",
            "position");

        char const* pointRole =
            (_geometricShader->GetPrimitiveType() ==
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS)
            ? "point_size" : "";

        HgiShaderFunctionAddStageOutput(
            &ptcsDesc, "gl_PointSize", "float",
                pointRole);

        if (!glslProgram->CompileShader(ptcsDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }

    if (_hasPTVS) {
        HgiShaderFunctionDesc ptvsDesc;
        ptvsDesc.shaderStage = HgiShaderStagePostTessellationVertex;

        ptvsDesc.tessellationDescriptor.numVertsPerPatchIn =
              std::to_string(_geometricShader->GetPrimitiveIndexSize());
        ptvsDesc.tessellationDescriptor.numVertsPerPatchOut =
              std::to_string(_geometricShader->GetNumPatchEvalVerts());

        //Set the patchtype to later decide tessfactor types
        ptvsDesc.tessellationDescriptor.patchType =
            (_geometricShader->IsPrimTypeTriangles() ||
              _geometricShader->GetPrimitiveType() ==
               HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE)
               ? HgiShaderFunctionTessellationDesc::PatchType::Triangles
               : HgiShaderFunctionTessellationDesc::PatchType::Quads;
        if (_geometricShader->GetHgiPrimitiveType() ==
            HgiPrimitiveTypePointList) {
            ptvsDesc.tessellationDescriptor.patchType =
                HgiShaderFunctionTessellationDesc::PatchType::Isolines;
        }

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptvsDesc,
            HdShaderTokens->postTessVertexShader, _resAttrib, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptvsDesc,
            HdShaderTokens->postTessVertexShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptvsDesc,
            HdShaderTokens->postTessVertexShader, _resPTVS, _GetMetaData());

        // material in PTVS
        resourceGen._GenerateHgiResources(registry->GetHgi(), &ptvsDesc,
            HdShaderTokens->postTessVertexShader, _resMaterial, _GetMetaData());
        resourceGen._GenerateHgiTextureResources(&ptvsDesc,
            HdShaderTokens->postTessVertexShader, _resTextures, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _genDecl.str() + _osd.str();
        std::string const source = _genAccessors.str() + _genPTVS.str();

        ptvsDesc.shaderCodeDeclarations = declarations.c_str();
        ptvsDesc.shaderCode = source.c_str();
        ptvsDesc.generatedShaderCodeOut = &_ptvsSource;

        // builtins

        HgiShaderFunctionAddStageInput(
            &ptvsDesc, "hd_BaseInstance", "uint",
            HgiShaderKeywordTokens->hdBaseInstance);
        HgiShaderFunctionAddStageInput(
            &ptvsDesc, "patch_id", "uint",
            HgiShaderKeywordTokens->hdPatchID);
        
        std::string tessCoordType =
            (_geometricShader->IsPrimTypeTriangles() ||
             _geometricShader->GetPrimitiveType() ==
               HdSt_GeometricShader::PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE)
            ? "vec3" : "vec2";
        
        HgiShaderFunctionAddStageInput(
            &ptvsDesc, "gl_TessCoord", tessCoordType,
            HgiShaderKeywordTokens->hdPositionInPatch);
        
        HgiShaderFunctionAddStageInput(
            &ptvsDesc, "hd_InstanceID", "uint",
            HgiShaderKeywordTokens->hdInstanceID);

        HgiShaderFunctionAddStageOutput(
            &ptvsDesc, "gl_Position", "vec4",
            "position");

        char const* pointRole =
            (_geometricShader->GetPrimitiveType() ==
            HdSt_GeometricShader::PrimitiveType::PRIM_POINTS)
            ? "point_size" : "";
        
        HgiShaderFunctionAddStageOutput(
            &ptvsDesc, "gl_PointSize", "float",
                pointRole);

        if (_hasClipPlanes) {
            HgiShaderFunctionAddStageOutput(
                &ptvsDesc, "gl_ClipDistance", "float",
                "clip_distance", /*arraySize*/"HD_NUM_clipPlanes");
        }

        if (!glslProgram->CompileShader(ptvsDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }

    if (_hasGS) {
        HgiShaderFunctionDesc gsDesc;
        gsDesc.shaderStage = HgiShaderStageGeometry;

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &gsDesc,
            HdShaderTokens->geometryShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &gsDesc,
            HdShaderTokens->geometryShader, _resGS, _GetMetaData());

        // material in GS
        resourceGen._GenerateHgiResources(registry->GetHgi(), &gsDesc,
            HdShaderTokens->geometryShader, _resMaterial, _GetMetaData());
        resourceGen._GenerateHgiTextureResources(&gsDesc,
            HdShaderTokens->geometryShader, _resTextures, _GetMetaData());

        std::string const declarations = _genDefines.str() + _genDecl.str() +
            _osd.str();
        std::string const source = _genAccessors.str() + _genGS.str();

        gsDesc.shaderCodeDeclarations = declarations.c_str();
        gsDesc.shaderCode = source.c_str();
        gsDesc.generatedShaderCodeOut = &_gsSource;

        if (_hasClipPlanes) {
            HgiShaderFunctionAddStageOutput(
                &gsDesc, "gl_ClipDistance", "float",
                "clip_distance", /*arraySize*/"HD_NUM_clipPlanes");
        }

        if (!glslProgram->CompileShader(gsDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }

    if (_hasFS) {
        HgiShaderFunctionDesc fsDesc;
        fsDesc.shaderStage = HgiShaderStageFragment;

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &fsDesc,
            HdShaderTokens->fragmentShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &fsDesc,
            HdShaderTokens->fragmentShader, _resFS, _GetMetaData());

        // material in FS
        resourceGen._GenerateHgiResources(registry->GetHgi(), &fsDesc,
            HdShaderTokens->fragmentShader, _resMaterial, _GetMetaData());
        resourceGen._GenerateHgiTextureResources(&fsDesc,
            HdShaderTokens->fragmentShader, _resTextures, _GetMetaData());

        std::string const declarations =
            _genDefines.str() + _genDecl.str() + _osd.str();
        std::string const source = _genAccessors.str() + _genFS.str();

        fsDesc.shaderCodeDeclarations = declarations.c_str();
        fsDesc.shaderCode = source.c_str();
        fsDesc.generatedShaderCodeOut = &_fsSource;

        // builtins
        HgiShaderFunctionAddStageInput(
            &fsDesc, "gl_PrimitiveID", "uint",
            HgiShaderKeywordTokens->hdPrimitiveID);
        HgiShaderFunctionAddStageInput(
            &fsDesc, "gl_FrontFacing", "bool",
            HgiShaderKeywordTokens->hdFrontFacing);
        HgiShaderFunctionAddStageInput(
            &fsDesc, "gl_FragCoord", "vec4",
            HgiShaderKeywordTokens->hdPosition);
        const bool builtinBarycentricsEnabled =
            registry->GetHgi()->GetCapabilities()->IsSet(
                HgiDeviceCapabilitiesBitsBuiltinBarycentrics);
        if (builtinBarycentricsEnabled) {
            HgiShaderFunctionAddStageInput(
                &fsDesc, "hd_BaryCoordNoPersp", "vec3",
                HgiShaderKeywordTokens->hdBaryCoordNoPersp);
        }

        if (!glslProgram->CompileShader(fsDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }

    if (_hasCS) {
        HgiShaderFunctionDesc csDesc;
        csDesc.shaderStage = HgiShaderStageCompute;

        _GenerateComputeParameters(&csDesc);

        resourceGen._AdvanceShaderStage();
        resourceGen._GenerateHgiResources(registry->GetHgi(), &csDesc,
            HdShaderTokens->computeShader, _resAttrib, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &csDesc,
            HdShaderTokens->computeShader, _resCommon, _GetMetaData());
        resourceGen._GenerateHgiResources(registry->GetHgi(), &csDesc,
            HdShaderTokens->computeShader, _resCS, _GetMetaData());

        std::string const declarations = _genDefines.str() + _genDecl.str();
        std::string const source = _genAccessors.str() + _genCS.str();

        csDesc.shaderCodeDeclarations = declarations.c_str();
        csDesc.shaderCode = source.c_str();
        csDesc.generatedShaderCodeOut = &_csSource;

        if (!glslProgram->CompileShader(csDesc)) {
            return nullptr;
        }

        shaderCompiled = true;
    }
    
    if (!shaderCompiled) {
        return nullptr;
    }

    return glslProgram;
}

static void _EmitDeclaration(
    HioGlslfxResourceLayout::ElementVector *elements,
    TfToken const &name,
    TfToken const &type,
    HdStBinding const &binding,
    bool isWritable,
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
    HdStBinding::Type bindingType = binding.GetType();

    if (!TF_VERIFY(!name.IsEmpty())) return;
    if (!TF_VERIFY(!type.IsEmpty(),
                      "Unknown dataType for %s",
                      name.GetText())) return;

    if (arraySize > 0) {
        if (!TF_VERIFY(bindingType == HdStBinding::UNIFORM_ARRAY             ||
                       bindingType == HdStBinding::DRAW_INDEX_INSTANCE_ARRAY ||
                       bindingType == HdStBinding::UBO                       ||
                       bindingType == HdStBinding::SSBO                      ||
                       bindingType == HdStBinding::BINDLESS_SSBO_RANGE       ||
                       bindingType == HdStBinding::BINDLESS_UNIFORM)) {
            // XXX: SSBO and BINDLESS_UNIFORM don't need arraySize, but for the
            // workaround of UBO allocation we're passing arraySize = 2
            // for all bindingType.
            return;
        }
    }

    // layout qualifier (if exists)
    uint32_t location = binding.GetLocation();
    switch (bindingType) {
        case HdStBinding::VERTEX_ATTR:
        case HdStBinding::DRAW_INDEX:
        case HdStBinding::DRAW_INDEX_INSTANCE:
            _AddVertexAttribElement(elements,
                                    /*name=*/name,
                                    /*dataType=*/_GetPackedType(type, false),
                                    location);

            break;
        case HdStBinding::DRAW_INDEX_INSTANCE_ARRAY:
        {
            for(int i = 0; i < arraySize;i++) {
                _AddVertexAttribElement(elements,
                                        /*name=*/TfToken(name.GetString() + std::to_string(i)),
                                        /*dataType=*/_GetPackedType(type, false),
                                        location + i);
            }
            break;
        }
        case HdStBinding::UNIFORM:
            _AddUniformValueElement(elements,
                                    /*name=*/name,
                                    /*dataType=*/_GetPackedType(type, true),
                                    location);
            break;
        case HdStBinding::UNIFORM_ARRAY:
            _AddUniformValueElement(elements,
                                    /*name=*/name,
                                    /*dataType=*/_GetPackedType(type, true),
                                    location,
                                    arraySize);
            break;
        case HdStBinding::UBO:
            _AddUniformBufferElement(elements,
                                     /*name=*/name,
                                     /*dataType=*/_GetPackedType(type, true),
                                     location,
                                     arraySize);
            break;
        case HdStBinding::SSBO:
            if (isWritable) {
                _AddWritableBufferElement(elements,
                                          /*name=*/name,
                                          /*type=*/_GetPackedType(type, true),
                                          location);
            } else {
                _AddBufferElement(elements,
                                  /*name=*/name,
                                  /*dataType=*/_GetPackedType(type, true),
                                  location);
            }
            break;
        case HdStBinding::BINDLESS_SSBO_RANGE:
            _AddUniformValueElement(elements,
                                    /*name=*/name,
                                    /*dataType=*/_GetPackedType(type, true),
                                    location);
            break;
        case HdStBinding::BINDLESS_UNIFORM:
            _AddUniformValueElement(elements,
                                    /*name=*/name,
                                    /*dataType=*/_GetPackedType(type, true),
                                    location);
            break;
        default:
            TF_CODING_ERROR("Unknown binding type %d, for %s\n",
                            binding.GetType(), name.GetText());
            break;
    }
}

static void _EmitDeclaration(
    HioGlslfxResourceLayout::ElementVector *elements,
    HdSt_ResourceBinder::MetaData::BindingDeclaration const &bindingDeclaration,
    int arraySize=0)
{
    _EmitDeclaration(elements,
                     bindingDeclaration.name,
                     bindingDeclaration.dataType,
                     bindingDeclaration.binding,
                     bindingDeclaration.isWritable,
                     arraySize);
}

static void _EmitStageAccessor(std::stringstream &str,
                               TfToken const &name,
                               std::string const &stageName,
                               TfToken const &type)
{
    str << _GetUnpackedType(type, false)
        << " HdGet_" << name << "(int localIndex) { return ";
    str << _GetPackedTypeAccessor(type, true) << "(" << stageName << ");}\n";

    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to localIndex=0
    str << _GetUnpackedType(type, false) << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";

    _EmitScalarAccessor(str, name, type);
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
    _EmitScalarAccessor(str, accessorName, type);
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

static bool _IsScalarType(TfToken const& type) {
    return (type == _tokens->_float ||
            type == _tokens->_int ||
            type == _tokens->_uint);
}

static std::string _GetSwizzleString(TfToken const& type, 
                                     std::string const& swizzle=std::string())
{
    if (!swizzle.empty()) {
        return "." + swizzle;
    } 
    if (type == _tokens->vec4 || type == _tokens->ivec4) {
        return "";
    }
    if (type == _tokens->vec3 || type == _tokens->ivec3) {
        return ".xyz";
    }
    if (type == _tokens->vec2 || type == _tokens->ivec2) {
        return ".xy";
    }
    if (_IsScalarType(type)) {
        return ".x";
    }
    if (type == _tokens->packed_2_10_10_10) {            
        return ".x";
    }

    return "";
}

static int _GetNumComponents(TfToken const& type)
{
    int numComponents = 1;
    if (type == _tokens->vec2 || type == _tokens->ivec2) {
        numComponents = 2;
    } else if (type == _tokens->vec3 || type == _tokens->ivec3) {
        numComponents = 3;
    } else if (type == _tokens->vec4 || type == _tokens->ivec4) {
        numComponents = 4;
    } else if (type == _tokens->mat3 || type == _tokens->dmat3) {
        numComponents = 9;
    } else if (type == _tokens->mat4 || type == _tokens->dmat4) {
        numComponents = 16;
    }

    return numComponents;
}

static void _EmitComputeAccessor(
                    std::stringstream &str,
                    TfToken const &name,
                    TfToken const &type,
                    HdStBinding const &binding,
                    const char *index)
{
    if (index) {
        str << _GetUnpackedType(type, false)
            << " HdGet_" << name << "(int localIndex) {\n";
        if (binding.GetType() == HdStBinding::SSBO) {
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
        } else if (binding.GetType() == HdStBinding::BINDLESS_SSBO_RANGE) {
            str << "  return " << _GetPackedTypeAccessor(type, true) << "("
                << name << "[localIndex]);\n}\n";
        } else {
            str << "  return " << _GetPackedTypeAccessor(type, true) << "("
                << name << "[localIndex]);\n}\n";
        }
    } else {
        // non-indexed, only makes sense for uniform or vertex.
        if (binding.GetType() == HdStBinding::UNIFORM || 
            binding.GetType() == HdStBinding::VERTEX_ATTR) {
            str << _GetUnpackedType(type, false)
                << " HdGet_" << name << "(int localIndex) { return ";
            str << _GetPackedTypeAccessor(type, true) << "(" << name << ");}\n";
        }
    }
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to localIndex=0
    str << _GetUnpackedType(type, false) << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";
    
}

static void _EmitComputeMutator(
                    std::stringstream &str,
                    TfToken const &name,
                    TfToken const &type,
                    HdStBinding const &binding,
                    const char *index)
{
    if (index) {
        str << "void"
            << " HdSet_" << name << "(int localIndex, "
            << _GetUnpackedType(type, false) << " value) {\n";
        if (binding.GetType() == HdStBinding::SSBO) {
            str << "  int index = " << index << ";\n";
            str << "  " << _GetPackedType(_ConvertBoolType(type), false) << " packedValue = "
                << _GetPackedTypeMutator(_ConvertBoolType(type), false) << "(value);\n";
            int numComponents = _GetNumComponents(_GetPackedType(_ConvertBoolType(type), false));
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
        } else if (binding.GetType() == HdStBinding::BINDLESS_SSBO_RANGE) {
            str << name << "[localIndex] = "
                << _GetPackedTypeMutator(_ConvertBoolType(type), true) << "(value);\n";
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
    // default to localIndex=0
    //str << "void HdSet_" << name << "(" << type << " value)"
    //    << " { HdSet_" << name << "(0, value); }\n";
    
}

static void _EmitScalarAccessor(std::stringstream &str,
                                TfToken const &name,
                                TfToken const &type)
{
    // Emit scalar accessors to support shading languages like MSL which
    // do not support swizzle operators on scalar values.
    if (_GetNumComponents(type) <= 4) {
        str << _GetFlatType(type) << " HdGetScalar_"
            << name << "(int localIndex)"
            << " { return HdGet_" << name << "(localIndex)"
            << _GetFlatTypeSwizzleString(type) << "; }\n";
        str << _GetFlatType(type) << " HdGetScalar_" << name << "()"
            << " { return HdGet_" << name << "(0)"
            << _GetFlatTypeSwizzleString(type) << "; }\n";
    }
}

static void _EmitAccessor(std::stringstream &str,
                          TfToken const &name,
                          TfToken const &type,
                          HdStBinding const &binding,
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
        if (binding.GetType() == HdStBinding::UNIFORM || 
            binding.GetType() == HdStBinding::VERTEX_ATTR) {
            str << _GetUnpackedType(type, false)
                << " HdGet_" << name << "(int localIndex) { return ";
            str << _GetPackedTypeAccessor(type, true) << "(" << name << ");}\n";
        }
    }
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to localIndex=0
    str << _GetUnpackedType(type, false) << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";

    _EmitScalarAccessor(str, name, type);
}

static void _EmitTextureAccessors(
    std::stringstream &accessors,
    HdSt_ResourceBinder::MetaData::ShaderParameterAccessor const &acc,
    std::string const &swizzle,
    std::string const &fallbackSwizzle,
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
            << "#ifdef HD_HAS_" << name << "_" 
            << HdStTokens->storm << "_" << HdStTokens->scale << "\n"
            << "FORWARD_DECL(vec4 HdGet_" << name << "_" 
            << HdStTokens->storm << "_" << HdStTokens->scale 
            << "());\n"
            << "#endif\n"
            << "#ifdef HD_HAS_" << name << "_" << HdStTokens->storm 
            << "_" << HdStTokens->bias  << "\n"
            << "FORWARD_DECL(vec4 HdGet_" << name << "_" << HdStTokens->storm 
            << "_" << HdStTokens->bias 
            << "());\n"
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
                    << "#define HdGetSampler_" << name << "(index) "
                    << "  HgiGetSampler_" << name << "(index)\n"
                    << "#define HdGetSize_" << name << "(index) "
                    << "  HgiGetSize_" << name << "(index)\n";
            } else {
                accessors
                    << "#define HdGetSampler_" << name << "() "
                    << "  HgiGetSampler_" << name << "()\n"
                    << "#define HdGetSize_" << name << "() "
                    << "  HgiGetSize_" << name << "()\n";
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

    if (!isBindless) {
        if (isArray) {
            accessors
            << _GetUnpackedType(dataType, false)
            << " HdTextureLod_" << name
            << "(int index, vec" << coordDim << " coord, float lod) {\n"
            << "  return " << _GetPackedTypeAccessor(dataType, false)
            << "(HgiTextureLod_" << name << "(index, coord, lod)"
            << swizzle << ");\n"
            << "}\n";
        } else {
            accessors
                << _GetUnpackedType(dataType, false)
                << " HdTextureLod_" << name
                << "(vec" << coordDim << " coord, float lod) {\n"
                << "  return " << _GetPackedTypeAccessor(dataType, false) 
                << "(HgiTextureLod_" << name << "(coord, lod)"
                << swizzle << ");\n"
                << "}\n";
        }
    } else {
        // bindless
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
        if (!isBindless) {
            accessors
                << "  " << _GetUnpackedType(dataType, false)
                << " result = "
                << _GetPackedTypeAccessor(dataType, false)
                << "((HgiGet_" << name;
            if (isArray) {
                accessors << "(index, sampleCoord)\n";
            } else {
                accessors << "(sampleCoord)\n";
            }
        } else {
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
        }
        accessors
            << "#ifdef HD_HAS_" << name << "_" << HdStTokens->storm << "_" 
            << HdStTokens->scale << "\n"
            << "    * HdGet_" << name << "_" << HdStTokens->storm << "_" 
            << HdStTokens->scale << "()\n"
            << "#endif\n" 
            << "#ifdef HD_HAS_" << name << "_" << HdStTokens->storm << "_" 
            << HdStTokens->bias << "\n"
            << "    + HdGet_" << name << "_" << HdStTokens->storm << "_" 
            << HdStTokens->bias  << "()\n"
            << "#endif\n"
            << ")" << swizzle << ");\n";
    } else {
        if (!isBindless) {
            accessors
                << "  " << _GetUnpackedType(dataType, false)
                << " result = "
                << _GetPackedTypeAccessor(dataType, false)
                << "(HgiGet_" << name;
            if (isArray) {
                accessors << "(index, sampleCoord)";
            } else {
                accessors << "(sampleCoord)";
            }
            accessors << swizzle << ");\n";
        } else {
            accessors
                << "  " << _GetUnpackedType(dataType, false)
                << " result = "
                << _GetPackedTypeAccessor(dataType, false)
                << "(texture(HdGetSampler_" << name;
            if (isArray) {
                accessors << "(index), sampleCoord)";
            } else {
                accessors << "(), sampleCoord)";
            }
            accessors << swizzle << ");\n";
        }
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
        accessors
            << "  if (bool(shaderData[shaderCoord]." << name
            << HdSt_ResourceBindingSuffixTokens->valid
            << ")) {\n";

        if (hasTextureScaleAndBias) {
            accessors
                << "    return result;\n"
                << "  } else {\n"
                << "    return ("
                << _GetPackedTypeAccessor(dataType, false)
                << "(shaderData[shaderCoord]."
                << name
                << HdSt_ResourceBindingSuffixTokens->fallback
                << fallbackSwizzle << ")\n"
                << "#ifdef HD_HAS_" << name << "_" 
                << HdStTokens->storm << "_" << HdStTokens->scale << "\n"
                << "        * HdGet_" << name << "_" 
                << HdStTokens->storm << "_" << HdStTokens->scale 
                << "()" << swizzle << "\n"
                << "#endif\n" 
                << "#ifdef HD_HAS_" << name << "_" 
                << HdStTokens->storm << "_" << HdStTokens->bias << "\n"
                << "        + HdGet_" << name << "_" 
                << HdStTokens->storm << "_" << HdStTokens->bias
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
                << HdSt_ResourceBindingSuffixTokens->fallback
                << fallbackSwizzle << ");\n"
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
            << "FORWARD_DECL(vec" << dim << " HdGet_" << inPrimvars[0] 
            << "(int localIndex));\n"
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
        << "() {\n  return HdGet_" << name << "(0);\n}\n";

    // float HdGetScalar_name()
    _EmitScalarAccessor(accessors, name, dataType);

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
                HdStBinding const &binding,
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
                <<           "HdGet_" << name << "_Coarse(indices[1]), st.x),"
                << "mix(" << "HdGet_" << name << "_Coarse(indices[3]),"
                <<           "HdGet_" << name << "_Coarse(indices[2]), st.x), "
                << "st.y);\n}\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_REFINED_TRIANGLES:
        {
            // barycentric interpolation between 3 refined primvars
            str << "  ivec3 indices = HdGet_fvarIndices" << fvarChannel 
                << "();\n"
                << "  return ("
                << "HdGet_" << name << "_Coarse(indices[0]) * (1-st.x-st.y)"
                << " + HdGet_" << name << "_Coarse(indices[1]) * st.x"
                << " + HdGet_" << name << "_Coarse(indices[2]) * st.y);\n}\n";
            break;
        }
        case HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE:
        case HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE:
        {
            // evaluation of a bspline/box spline patch
            str << "  ivec2 fvarPatchParam = HdGet_fvarPatchParam" 
                << fvarChannel << "();\n"
                << "  OsdPatchParam param = OsdPatchParamInit(fvarPatchParam.x,"
                << " fvarPatchParam.y, 0.0f);\n"
                << "  float wP[20], wDu[20], wDv[20], wDuu[20], wDuv[20], "
                << "wDvv[20];\n"
                << "  OsdEvaluatePatchBasisNormalized(patchType, param,"
                << " st.x, st.y, wP, wDu, wDv, wDuu, wDuv, wDvv);\n"
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

    str << "FORWARD_DECL(vec4 GetPatchCoord(int index));\n"
        << "FORWARD_DECL(vec2 GetPatchCoordLocalST());\n"
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
                << " fvarPatchParam.y, 0.0f);\n"
                << "  vec2 unnormalized = GetPatchCoord(localIndex).xy;\n"
                << "  float uv[2] = { unnormalized.x, unnormalized.y };\n"
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
                << " fvarPatchParam.y, 0.0f);\n"
                << "  vec2 unnormalized = GetPatchCoord(localIndex).xy;\n"
                << "  float uv[2] = { unnormalized.x, unnormalized.y };\n"
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

// Helper function to generate the implementation of "GetDrawingCoord()".
static void
_GetDrawingCoord(std::stringstream &ss,
                 std::vector<std::string> const &drawingCoordParams,
                 int const instanceIndexWidth,
                 char const *inputPrefix,
                 char const *inArraySize)
{
    ss << "hd_drawingCoord GetDrawingCoord() { \n"
       << "  hd_drawingCoord dc; \n";

    for (std::string const & param : drawingCoordParams) {
        ss << "  dc." << param
           << " = " << inputPrefix << param << inArraySize << ";\n";
    }
    for(int i = 0; i < instanceIndexWidth; ++i) {
        ss << "  dc.instanceIndex[" << std::to_string(i) << "]"
           << " = " << inputPrefix
           << "instanceIndexI" << std::to_string(i) << inArraySize << ";\n";
    }
    for(int i = 0; i < instanceIndexWidth-1; ++i) {
        ss << "  dc.instanceCoords[" << std::to_string(i) << "]"
           << " = " << inputPrefix
           << "instanceCoordsI" << std::to_string(i) << inArraySize << ";\n";
    }

    ss << "  return dc; \n"
       << "}\n";
}

// Helper function to generate drawingCoord interstage processing.
static void
_ProcessDrawingCoord(std::stringstream &ss,
                     std::vector<std::string> const &drawingCoordParams,
                     int const instanceIndexWidth,
                     char const *outputPrefix,
                     char const *outArraySize)
{
    ss << "  hd_drawingCoord dc = GetDrawingCoord();\n";
    for (std::string const & param : drawingCoordParams) {
        ss << "  " << outputPrefix << param << outArraySize
           << " = " << "dc." << param << ";\n";
    }
    for(int i = 0; i < instanceIndexWidth; ++i) {
        std::string const index = std::to_string(i);
        ss << "  " << outputPrefix << "instanceIndexI" << index << outArraySize
           << " = " << "dc.instanceIndex[" << index << "]" << ";\n";
    }
    for(int i = 0; i < instanceIndexWidth-1; ++i) {
        std::string const index = std::to_string(i);
        ss << "  " << outputPrefix << "instanceCoordsI" << index << outArraySize
           << " = " << "dc.instanceCoords[" << index << "]" << ";\n";
    }
}

void
HdSt_CodeGen::_GenerateDrawingCoord(
    bool const shaderDrawParametersEnabled,
    bool const requiresBasePrimitiveOffset,
    bool const requiresPrimitiveIdEmulation)
{
    TF_VERIFY(_metaData->drawingCoord0Binding.binding.IsValid());
    TF_VERIFY(_metaData->drawingCoord1Binding.binding.IsValid());
    TF_VERIFY(_metaData->drawingCoord2Binding.binding.IsValid());

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

    static const std::vector<std::string> drawingCoordParams {
        "modelCoord",
        "constantCoord",
        "elementCoord",
        "primitiveCoord",
        "fvarCoord",
        "shaderCoord",
        "vertexCoord",
        "topologyVisibilityCoord",
        "varyingCoord"
    };

    // common
    //
    // note: instanceCoords should be [HD_INSTANCER_NUM_LEVELS], but since
    //       GLSL doesn't allow [0] declaration, we use +1 value (WIDTH)
    //       for the sake of simplicity.
    _genDecl << "struct hd_drawingCoord {                       \n";
    for (std::string const & param : drawingCoordParams) {
        _genDecl << "  int " << param << ";\n";
    }
    _genDecl <<"  int instanceIndex[HD_INSTANCE_INDEX_WIDTH];\n";
    _genDecl <<"  int instanceCoords[HD_INSTANCE_INDEX_WIDTH];\n";
    _genDecl << "};\n";

    // forward declaration
    _genDecl << "FORWARD_DECL(hd_drawingCoord GetDrawingCoord());\n"
                "FORWARD_DECL(int HgiGetBaseVertex());\n";

    int instanceIndexWidth = _metaData->instancerNumLevels + 1;

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
    if (!_hasCS) {
        _EmitDeclaration(&_resAttrib, _metaData->drawingCoord0Binding);
        _EmitDeclaration(&_resAttrib, _metaData->drawingCoord1Binding);
        _EmitDeclaration(&_resAttrib, _metaData->drawingCoord2Binding);

        if (_metaData->drawingCoordIBinding.binding.IsValid()) {
            _EmitDeclaration(&_resAttrib, _metaData->drawingCoordIBinding,
                /*arraySize=*/std::max(1, _metaData->instancerNumLevels));
        }
    }

    std::stringstream primitiveID;

    if(_hasPTVS) {
        // A driver bug that emits the wrong primitive ID based on the first
        // patch instance offset exists on Apple Silicon. Use primitiveCoord
        // subtracted from the primitive ID for those cases
        if (requiresBasePrimitiveOffset) {
            primitiveID << "int GetBasePrimitiveOffset() { return vs_dc_primitiveCoord; }\n";
            _genPTCS    << "int GetBasePrimitiveOffset() { return drawingCoord0[0].w; }\n";
            _genPTVS    << "int GetBasePrimitiveOffset() { return drawingCoord0[0].w; }\n";
        } else {
            primitiveID << "int GetBasePrimitiveOffset() { return 0; }\n";
            _genPTCS    << "int GetBasePrimitiveOffset() { return 0; }\n";
            _genPTVS    << "int GetBasePrimitiveOffset() { return 0; }\n";
        }
        // A driver bug causes primitive_id in FS to be incorrect when PTVS
        // is active. As a workaround we plumb patch_id from PTVS to FS.
        if (requiresPrimitiveIdEmulation) {
            primitiveID << "int GetBasePrimitiveId() { return hd_patchID; }\n";
        } else {
            primitiveID << "int GetBasePrimitiveId() { return gl_PrimitiveID; }\n";
        }
        if (HdSt_GeometricShader::IsPrimTypeTriQuads(_geometricShader->GetPrimitiveType())) {
            primitiveID << "int GetPrimitiveID() {\n"
                        << "  return (GetBasePrimitiveId() - GetBasePrimitiveOffset());\n"
                        << "}\n"
                        << "int GetTriQuadID() {\n"
                        << "  return (GetBasePrimitiveId() - GetBasePrimitiveOffset()) & 1;\n"
                        << "}\n";
            _genPTCS    << "int GetPrimitiveID() {\n"
                        << "  return (patch_id - GetBasePrimitiveOffset()) / 2;\n"
                        << "}\n"
                        << "int GetTriQuadID() {\n"
                        << "  return (patch_id - GetBasePrimitiveOffset()) & 1;\n"
                        << "}\n";
            _genPTVS    << "int GetPrimitiveID() {\n"
                        << "  return (patch_id - GetBasePrimitiveOffset()) / 2;\n"
                        << "}\n"
                        << "int GetTriQuadID() {\n"
                        << "  return (patch_id - GetBasePrimitiveOffset()) & 1;\n"
                        << "}\n";
        } else {
            primitiveID << "int GetPrimitiveID() {\n"
                        << "  return (GetBasePrimitiveId() - GetBasePrimitiveOffset());\n"
                        << "}\n";
            _genPTCS    << "int GetPrimitiveID() {\n"
                        << "  return (patch_id - GetBasePrimitiveOffset());\n"
                        << "}\n";
            _genPTVS    << "int GetPrimitiveID() {\n"
                        << "  return (patch_id - GetBasePrimitiveOffset());\n"
                        << "}\n";
        }
    } else {
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
    }

    _genTCS << primitiveID.str();
    _genTES << primitiveID.str();
    _genGS << primitiveID.str();
    _genFS << primitiveID.str();

    // To access per-primitive data we need the primitiveCoord offset
    // to the start of primitive data for the current draw added to
    // the PrimitiveID offset to current primitive within the draw.
    // We don't generate this accessor for VS since VS does not
    // support PrimitiveID.
    char const * const primitiveIndex =
        "int GetPrimitiveIndex() {\n"
        "  return GetDrawingCoord().primitiveCoord + GetPrimitiveID();\n"
        "}\n";

    // For PTCS/PTVS we index by patch_id when using GS emulation.
    char const * const primitiveIndexFromPatchID =
        "int GetPrimitiveIndex() {\n"
        "  return GetDrawingCoord().primitiveCoord + patch_id;\n"
        "}\n";

    if (!_geometricShader->IsPrimTypePatches()) {
        _genPTCS << primitiveIndexFromPatchID;
        _genPTVS << primitiveIndexFromPatchID;
    } else {
        _genPTCS << primitiveIndex;
        _genPTVS << primitiveIndex;
    }

    _genTCS << primitiveIndex;
    _genTES << primitiveIndex;
    _genGS << primitiveIndex;
    _genFS << primitiveIndex;

    std::stringstream genAttr;

    // VS/PTVS specific accessor for the "vertex drawing coordinate"
    // Even though we currently always plumb vertexCoord as part of the drawing
    // coordinate, we expect clients to use this accessor when querying the base
    // vertex offset for a draw call.
    genAttr << "int GetBaseVertexOffset() {\n";
    if (shaderDrawParametersEnabled) {
        genAttr << "  return HgiGetBaseVertex();\n";
    } else {
        genAttr << "  return GetDrawingCoord().vertexCoord;\n";
    }
    genAttr << "}\n";

    // instance index indirection
    _genDecl << "struct hd_instanceIndex { int indices[HD_INSTANCE_INDEX_WIDTH]; };\n";

    if (_hasCS) {
        // In order to access the drawing coordinate from CS the compute
        // shader needs to specify the current draw and current instance.
        _genCS << "struct hd_DrawIndex {\n"
               << "  int drawId;\n"
               << "  int instanceId;\n"
               << "} hd_drawIndex;\n\n"

               << "void SetDrawIndex(int drawId, int instanceId) {\n"
               << "  hd_drawIndex.drawId = drawId;\n"
               << "  hd_drawIndex.instanceId = instanceId;\n"
               << "}\n\n"

               << "int GetDrawingCoordField(int offset) {\n"
               << "  const int drawIndexOffset = "
               << _metaData->drawingCoordBufferBinding.offset
               << ";\n"

               << "  const int drawIndexStride = "
               << _metaData->drawingCoordBufferBinding.stride
               << ";\n"

               << "  const int base = "
               << "hd_drawIndex.drawId * drawIndexStride + drawIndexOffset;\n"
               << "  return int("
               << _metaData->drawingCoordBufferBinding.bufferName
               << "[base + offset]);\n"
               << "}\n";
    }

    if (_metaData->instanceIndexArrayBinding.binding.IsValid()) {
        // << layout (location=x) uniform (int|ivec[234]) *instanceIndices;
        _EmitDeclaration(&_resCommon, _metaData->instanceIndexArrayBinding);

        // << layout (location=x) uniform (int|ivec[234]) *culledInstanceIndices;
        HdSt_ResourceBinder::MetaData::BindingDeclaration const &
                bindingDecl = _metaData->culledInstanceIndexArrayBinding;
        _EmitDeclaration(&_resCommon, bindingDecl);

        /// if \p cullingPass is true, CodeGen generates GetInstanceIndex()
        /// such that it refers instanceIndices buffer (before culling).
        /// Otherwise, GetInstanceIndex() looks up culledInstanceIndices.

        _genVS << "int GetBaseInstanceIndexCoord() {\n"
               << "  return drawingCoord1.y;\n"
               << "}\n"

               << "int GetCurrentInstance() {\n"
               << "  return int(hd_InstanceID - hd_BaseInstance);\n"
               << "}\n"

               << "int GetInstanceIndexCoord() {\n"
               << "  return GetBaseInstanceIndexCoord() +"
               << " GetCurrentInstance() * HD_INSTANCE_INDEX_WIDTH;\n"
               << "}\n";
        
        _genPTCS << "int GetBaseInstanceIndexCoord() {\n"
               << "  return drawingCoord1[0].y;\n"
               << "}\n"

               << "int GetCurrentInstance() {\n"
               << "  return int(hd_InstanceID - hd_BaseInstance);\n"
               << "}\n"

               << "int GetInstanceIndexCoord() {\n"
               << "  return GetBaseInstanceIndexCoord() +"
               << " GetCurrentInstance() * HD_INSTANCE_INDEX_WIDTH;\n"
               << "}\n";

        _genPTVS << "int GetBaseInstanceIndexCoord() {\n"
               << "  return drawingCoord1[0].y;\n"
               << "}\n"

               << "int GetCurrentInstance() {\n"
               << "  return int(hd_InstanceID - hd_BaseInstance);\n"
               << "}\n"

               << "int GetInstanceIndexCoord() {\n"
               << "  return GetBaseInstanceIndexCoord() +"
               << " GetCurrentInstance() * HD_INSTANCE_INDEX_WIDTH;\n"
               << "}\n";

        _genCS << "int GetBaseInstanceIndexCoord() {\n"
               << "  return GetDrawingCoordField(5);\n"
               << "}\n"

               << "int GetCurrentInstance() {\n"
               << "  return hd_drawIndex.instanceId;\n"
               << "}\n"

               << "int GetInstanceIndexCoord() {\n"
               << "  return GetBaseInstanceIndexCoord() + "
               << " GetCurrentInstance() * HD_INSTANCE_INDEX_WIDTH;\n"
               << "}\n";

        if (_geometricShader->IsFrustumCullingPass()) {
            // for frustum culling:  use instanceIndices.
            char const *instanceIndexAccessors =
                "hd_instanceIndex GetInstanceIndex() {\n"
                "  int offset = GetInstanceIndexCoord();\n"
                "  hd_instanceIndex r;\n"
                "  for (int i = 0; i < HD_INSTANCE_INDEX_WIDTH; ++i)\n"
                "    r.indices[i] = instanceIndices[offset+i + 1];\n"
                "  return r;\n"
                "}\n"

                "void SetCulledInstanceIndex(uint instanceID) {\n"
                "  for (int i = 0; i < HD_INSTANCE_INDEX_WIDTH; ++i)\n"
                "    culledInstanceIndices[GetBaseInstanceIndexCoord()"
                " + instanceID*HD_INSTANCE_INDEX_WIDTH + i + 1]"
                "        = instanceIndices[GetBaseInstanceIndexCoord()"
                " + GetCurrentInstance()*HD_INSTANCE_INDEX_WIDTH + i + 1];\n"
                "}\n";

            genAttr << instanceIndexAccessors;

            _genCS << instanceIndexAccessors;

        } else {
            // for drawing:  use culledInstanceIndices.
            _EmitAccessor(_genVS, _metaData->culledInstanceIndexArrayBinding.name,
                          _metaData->culledInstanceIndexArrayBinding.dataType,
                          _metaData->culledInstanceIndexArrayBinding.binding,
                          "GetInstanceIndexCoord()+localIndex + 1");
            _EmitAccessor(_genPTCS, _metaData->culledInstanceIndexArrayBinding.name,
                          _metaData->culledInstanceIndexArrayBinding.dataType,
                          _metaData->culledInstanceIndexArrayBinding.binding,
                          "GetInstanceIndexCoord()+localIndex + 1");
            _EmitAccessor(_genPTVS, _metaData->culledInstanceIndexArrayBinding.name,
                          _metaData->culledInstanceIndexArrayBinding.dataType,
                          _metaData->culledInstanceIndexArrayBinding.binding,
                          "GetInstanceIndexCoord()+localIndex + 1");

            genAttr << "hd_instanceIndex GetInstanceIndex() {\n"
                    << "  hd_instanceIndex r;\n"
                    << "  for (int i = 0; i < HD_INSTANCE_INDEX_WIDTH; ++i)\n"
                    << "    r.indices[i] = HdGet_culledInstanceIndices(/*localIndex=*/i);\n"
                    << "  return r;\n"
                    << "}\n";
        }
    } else {
        genAttr << "hd_instanceIndex GetInstanceIndex() {"
             << "  hd_instanceIndex r; r.indices[0] = 0; return r; }\n";
        if (_geometricShader->IsFrustumCullingPass()) {
            genAttr << "void SetCulledInstanceIndex(uint instance) "
                    "{ /*no-op*/ }\n";
        }

        _genCS << "hd_instanceIndex GetInstanceIndex() {"
               << "  hd_instanceIndex r; r.indices[0] = 0; return r; }\n";
    }

    if (!_hasCS) {
        for (std::string const & param : drawingCoordParams) {
            TfToken const drawingCoordParamName("dc_" + param);
            _AddInterstageElement(&_resInterstage,
                                  HioGlslfxResourceLayout::InOut::NONE,
                                  /*name=*/drawingCoordParamName,
                                  /*dataType=*/_tokens->_int);
        }
        for (int i = 0; i < instanceIndexWidth; ++i) {
            TfToken const name(TfStringPrintf("dc_instanceIndexI%d", i));
            _AddInterstageElement(&_resInterstage,
                                  HioGlslfxResourceLayout::InOut::NONE,
                                  /*name=*/name,
                                  /*dataType=*/_tokens->_int);
        }
        for (int i = 0; i < instanceIndexWidth; ++i) {
            TfToken const name(TfStringPrintf("dc_instanceCoordsI%d", i));
            _AddInterstageElement(&_resInterstage,
                                  HioGlslfxResourceLayout::InOut::NONE,
                                  /*name=*/name,
                                  /*dataType=*/_tokens->_int);
        }
    }

    _genVS   << genAttr.str();
    _genPTCS << genAttr.str();
    _genPTVS << genAttr.str();

    _genVS   << "hd_drawingCoord GetDrawingCoord() { hd_drawingCoord dc;\n"
             << "  dc.modelCoord              = drawingCoord0.x;\n"
             << "  dc.constantCoord           = drawingCoord0.y;\n"
             << "  dc.elementCoord            = drawingCoord0.z;\n"
             << "  dc.primitiveCoord          = drawingCoord0.w;\n"
             << "  dc.fvarCoord               = drawingCoord1.x;\n"
             << "  dc.shaderCoord             = drawingCoord1.z;\n"
             << "  dc.vertexCoord             = drawingCoord1.w;\n"
             << "  dc.topologyVisibilityCoord = drawingCoord2.x;\n"
             << "  dc.varyingCoord            = drawingCoord2.y;\n"
             << "  hd_instanceIndex r = GetInstanceIndex();\n";

    _genPTCS << "hd_drawingCoord GetDrawingCoord() { hd_drawingCoord dc;\n"
             << "  dc.modelCoord              = drawingCoord0[0].x;\n"
             << "  dc.constantCoord           = drawingCoord0[0].y;\n"
             << "  dc.elementCoord            = drawingCoord0[0].z;\n"
             << "  dc.primitiveCoord          = drawingCoord0[0].w;\n"
             << "  dc.fvarCoord               = drawingCoord1[0].x;\n"
             << "  dc.shaderCoord             = drawingCoord1[0].z;\n"
             << "  dc.vertexCoord             = drawingCoord1[0].w;\n"
             << "  dc.topologyVisibilityCoord = drawingCoord2[0].x;\n"
             << "  dc.varyingCoord            = drawingCoord2[0].y;\n"
             << "  hd_instanceIndex r = GetInstanceIndex();\n";

    _genPTVS << "hd_drawingCoord GetDrawingCoord() { hd_drawingCoord dc;\n"
             << "  dc.modelCoord              = drawingCoord0[0].x;\n"
             << "  dc.constantCoord           = drawingCoord0[0].y;\n"
             << "  dc.elementCoord            = drawingCoord0[0].z;\n"
             << "  dc.primitiveCoord          = drawingCoord0[0].w;\n"
             << "  dc.fvarCoord               = drawingCoord1[0].x;\n"
             << "  dc.shaderCoord             = drawingCoord1[0].z;\n"
             << "  dc.vertexCoord             = drawingCoord1[0].w;\n"
             << "  dc.topologyVisibilityCoord = drawingCoord2[0].x;\n"
             << "  dc.varyingCoord            = drawingCoord2[0].y;\n"
             << "  hd_instanceIndex r = GetInstanceIndex();\n";

    _genCS   << "// Compute shaders read the drawCommands buffer directly.\n"
             << "hd_drawingCoord GetDrawingCoord() {\n"
             << "  hd_drawingCoord dc;\n"
             << "  dc.modelCoord              = GetDrawingCoordField(0);\n"
             << "  dc.constantCoord           = GetDrawingCoordField(1);\n"
             << "  dc.elementCoord            = GetDrawingCoordField(2);\n"
             << "  dc.primitiveCoord          = GetDrawingCoordField(3);\n"
             << "  dc.fvarCoord               = GetDrawingCoordField(4);\n"
             << "  dc.shaderCoord             = GetDrawingCoordField(6);\n"
             << "  dc.vertexCoord             = GetDrawingCoordField(7);\n"
             << "  dc.topologyVisibilityCoord = GetDrawingCoordField(8);\n"
             << "  dc.varyingCoord            = GetDrawingCoordField(9);\n"
             << "  hd_instanceIndex r = GetInstanceIndex();\n";

    for(int i = 0; i < instanceIndexWidth; ++i) {
        std::string const index = std::to_string(i);
        _genVS   << "  dc.instanceIndex[" << index << "]"
                 << " = r.indices[" << index << "];\n";
        _genPTCS << "  dc.instanceIndex[" << index << "]"
                 << " = r.indices[" << index << "];\n";
        _genPTVS << "  dc.instanceIndex[" << index << "]"
                 << " = r.indices[" << index << "];\n";
        _genCS   << "  dc.instanceIndex[" << index << "]"
                 << " = r.indices[" << index << "];\n";
    }
    for(int i = 0; i < instanceIndexWidth-1; ++i) {
        std::string const index = std::to_string(i);
        _genVS   << "  dc.instanceCoords[" << index << "]"
                 << " = drawingCoordI" << index << ""
                 << " + dc.instanceIndex[" << std::to_string(i+1) << "];\n";
        _genPTCS << "  dc.instanceCoords[" << index << "]"
                 << " = drawingCoordI" << index << "[0]"
                 << " + dc.instanceIndex[" << std::to_string(i+1) << "];\n";
        _genPTVS << "  dc.instanceCoords[" << index << "]"
                 << " = drawingCoordI" << index << "[0]"
                 << " + dc.instanceIndex[" << std::to_string(i+1) << "];\n";
        _genCS   << "  dc.instanceCoords[" << index << "]"
                 << " = GetDrawingCoordField(10 + " << index << ")"
                 << " + dc.instanceIndex[" << std::to_string(i+1) << "];\n";
    }

    _genVS   << "  return dc;\n"
             << "}\n";

    _genPTCS << "  return dc;\n"
             << "}\n";
    _genPTVS << "  return dc;\n"
             << "}\n";
    _genCS   << "  return dc;\n"
             << "}\n";

    // note: GL spec says tessellation input array size must be equal to
    //       gl_MaxPatchVertices, which is used for intrinsic declaration
    //       of built-in variables:
    //       in gl_PerVertex {} gl_in[gl_MaxPatchVertices];

    // drawing coord plumbing.
    // Note that copying from [0] for multiple input source since the
    // drawingCoord is flat (no interpolation required).

    // VS/PTVS from attributes
    _ProcessDrawingCoord(_procVS, drawingCoordParams, instanceIndexWidth,
                         "vs_dc_", "");
    _ProcessDrawingCoord(_procPTVSOut, drawingCoordParams, instanceIndexWidth,
                         "vs_dc_", "");

    // TCS from VS
    if (_hasTCS) {
        _GetDrawingCoord(_genTCS, drawingCoordParams, instanceIndexWidth,
                "vs_dc_", "[0]");
        _ProcessDrawingCoord(_procTCS, drawingCoordParams, instanceIndexWidth,
                "tcs_dc_", "[gl_InvocationID]");
    }

    // TES from TCS
    if (_hasTES) {
        _GetDrawingCoord(_genTES, drawingCoordParams, instanceIndexWidth,
                "tcs_dc_", "[0]");
        _ProcessDrawingCoord(_procTES, drawingCoordParams, instanceIndexWidth,
                "tes_dc_", "");
    }

    // GS
    if (_hasGS && _hasTES) {
        // from TES
        _GetDrawingCoord(_genGS, drawingCoordParams, instanceIndexWidth,
                "tes_dc_", "[0]");
    } else if (_hasGS) {
        // from VS
        _GetDrawingCoord(_genGS, drawingCoordParams, instanceIndexWidth,
                "vs_dc_", "[0]");
    }
    _ProcessDrawingCoord(_procGS, drawingCoordParams, instanceIndexWidth,
                "gs_dc_", "");

    // FS
    if (_hasGS) {
        // from GS
        _GetDrawingCoord(_genFS, drawingCoordParams, instanceIndexWidth,
                "gs_dc_", "");
    } else if (_hasTES) {
        // from TES
        _GetDrawingCoord(_genFS, drawingCoordParams, instanceIndexWidth,
                "tes_dc_", "");
    } else {
        // from VS/PTVS
        _GetDrawingCoord(_genFS, drawingCoordParams, instanceIndexWidth,
                "vs_dc_", "");
    }
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

    TF_FOR_ALL (it, _metaData->constantData) {
        // note: _constantData has been sorted by offset in HdSt_ResourceBinder.
        // XXX: not robust enough, should consider padding and layouting rules
        // to match with the logic in HdInterleavedMemoryManager if we
        // want to use a layouting policy other than default padding.

        HdStBinding binding = it->first;
        TfToken typeName(TfStringPrintf("ConstantData%d", binding.GetValue()));
        TfToken varName = it->second.blockName;

        _genDecl << "struct " << typeName << " {\n";

        TF_FOR_ALL (dbIt, it->second.entries) {
            if (!TF_VERIFY(!dbIt->dataType.IsEmpty(),
                              "Unknown dataType for %s",
                              dbIt->name.GetText())) {
                continue;
            }

            _genDecl << "  " << _GetPackedType(dbIt->dataType, false)
                         << " " << dbIt->name;
            if (dbIt->arraySize > 1) {
                _genDecl << "[" << dbIt->arraySize << "]";
            }

            _genDecl << ";\n";

            _EmitStructAccessor(_genAccessors, varName, dbIt->name, dbIt->dataType,
                                dbIt->arraySize,
                                "GetDrawingCoord().constantCoord");
        }
        _genDecl << "};\n";

        _EmitDeclaration(&_resCommon, varName, typeName, binding, 
            /*writable=*/false, /*arraySize=*/1);
    }
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
      vec3 HdGet_hydra_instanceTranslations(int localIndex=0) {
          return instanceData0[GetInstanceCoord()].translate;
      }
    */

    std::stringstream accessors;

    struct LevelEntries {
        TfToken dataType;
        std::vector<int> levels;
    };
    std::map<TfToken, LevelEntries> nameAndLevels;

    TF_FOR_ALL (it, _metaData->instanceData) {
        HdStBinding binding = it->first;
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
        _EmitDeclaration(&_resCommon, name, dataType, binding);
        _EmitAccessor(accessors, name, dataType, binding, n.str().c_str());
    }

    /*
      accessor taking level as a parameter.
      note that instance primvar may or may not be defined for each level.
      we expect level is an unrollable constant to optimize out branching.

      vec3 HdGetInstance_hydra_instanceTranslations(int level, vec3 defaultValue) {
          if (level == 0) return HdGet_hydra_instanceTranslations_0();
          // level==1 is not defined. use default
          if (level == 2) return HdGet_hydra_instanceTranslations_2();
          if (level == 3) return HdGet_hydra_instanceTranslations_3();
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

      #if !defined(HD_HAS_hydra_instanceTranslations)
      #define HD_HAS_hydra_instanceTranslations 1
      vec3 HdGet_hydra_instanceTranslations(int localIndex) {
          // 0 is the lowest level for which this is defined
          return HdGet_hydra_instanceTranslations_0();
      }
      vec3 HdGet_hydra_instanceTranslations() {
          return HdGet_hydra_instanceTranslations(0);
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

    _genAccessors << accessors.str();
}

void
HdSt_CodeGen::_GenerateElementPrimvar()
{
    // Don't need to codegen element primvars for frustum culling as they're
    // unneeded. Including them can cause errors in Hgi backends like Vulkan, 
    // which needs the resource layout made in HgiVulkanResourceBindings to 
    // match the one generated by SPIRV-Reflect in HgiVulkanGraphicsPipeline
    // when creating the VkPipelineLayout.
    if (_geometricShader->IsFrustumCullingPass()) {
        return;
    }
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
          return primitiveData[GetPrimitiveIndex()].elementID;
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

    std::stringstream accessors;

    if (_metaData->primitiveParamBinding.binding.IsValid()) {

        HdStBinding binding = _metaData->primitiveParamBinding.binding;
        _EmitDeclaration(&_resCommon, _metaData->primitiveParamBinding);
        _EmitAccessor(accessors, _metaData->primitiveParamBinding.name,
                        _metaData->primitiveParamBinding.dataType, binding,
                        "GetPrimitiveIndex()");

        if (_geometricShader->IsPrimTypeCompute()) {
            // do nothing.
        }
        else if (_geometricShader->IsPrimTypePoints()) {
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
                        << "FORWARD_DECL(int HdGetScalar_coarseFaceIndex());\n"
                        << "#endif\n"
                        << "ivec3 GetPatchParam() {\n"
                        << "#if defined(HD_HAS_coarseFaceIndex)\n "
                        << "  return ivec3(HdGetScalar_coarseFaceIndex(), 0, 0);\n"
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
    _genDecl
        << "FORWARD_DECL(int GetElementID());\n"
        << "FORWARD_DECL(int GetAggregatedElementID());\n";


    if (_metaData->edgeIndexBinding.binding.IsValid()) {

        HdStBinding binding = _metaData->edgeIndexBinding.binding;

        _EmitDeclaration(&_resCommon, _metaData->edgeIndexBinding);
        _EmitAccessor(accessors, _metaData->edgeIndexBinding.name,
                    _metaData->edgeIndexBinding.dataType, binding,
                    "GetPrimitiveIndex()");
    }

    if (_metaData->coarseFaceIndexBinding.binding.IsValid()) {
        _genDefines << "#define HD_HAS_" 
            << _metaData->coarseFaceIndexBinding.name << " 1\n";

        const HdStBinding &binding = _metaData->coarseFaceIndexBinding.binding;

        _EmitDeclaration(&_resCommon, _metaData->coarseFaceIndexBinding);
        _EmitAccessor(accessors, _metaData->coarseFaceIndexBinding.name,
                    _metaData->coarseFaceIndexBinding.dataType, binding,
                    "GetPrimitiveIndex() + localIndex");
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

    _genDecl
        << "FORWARD_DECL(int GetPrimitiveEdgeId());\n"
        << "FORWARD_DECL(float GetSelectedEdgeOpacity());\n";

    // Uniform primvar data declarations & accessors
    if (!_geometricShader->IsPrimTypePoints()) {
        TF_FOR_ALL (it, _metaData->elementData) {
            HdStBinding binding = it->first;
            TfToken const &name = it->second.name;
            TfToken const &dataType = it->second.dataType;

            _EmitDeclaration(&_resCommon, name, dataType, binding);
            // AggregatedElementID gives us the buffer index post batching, which
            // is what we need for accessing element (uniform) primvar data.
            _EmitAccessor(accessors, name, dataType, binding,"GetAggregatedElementID()");
        }
    }

    for (size_t i = 0; i < _metaData->fvarIndicesBindings.size(); ++i) {
        if (!_metaData->fvarIndicesBindings[i].binding.IsValid()) {
            continue;
        }

        HdStBinding binding = _metaData->fvarIndicesBindings[i].binding;
        TfToken name = _metaData->fvarIndicesBindings[i].name;
        _EmitDeclaration(&_resCommon, name, 
            _metaData->fvarIndicesBindings[i].dataType, 
            _metaData->fvarIndicesBindings[i].binding, 0);

        if (_geometricShader->GetFvarPatchType() == 
            HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE || 
            _geometricShader->GetFvarPatchType() ==
            HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
            _EmitAccessor(accessors, name,
                _metaData->fvarIndicesBindings[i].dataType, binding,
                "GetPrimitiveIndex() * HD_NUM_PATCH_VERTS + localIndex");
        } else {
            _EmitAccessor(accessors,name,
                _metaData->fvarIndicesBindings[i].dataType, binding,
                "GetPrimitiveIndex() + localIndex");
        }
    }

    for (size_t i = 0; i < _metaData->fvarPatchParamBindings.size(); ++i) {
        if (!_metaData->fvarPatchParamBindings[i].binding.IsValid()) {
            continue;
        }

        HdStBinding binding = _metaData->fvarPatchParamBindings[i].binding;
        TfToken name = _metaData->fvarPatchParamBindings[i].name;
        _EmitDeclaration(&_resCommon, name, 
            _metaData->fvarPatchParamBindings[i].dataType, 
            _metaData->fvarPatchParamBindings[i].binding, 0);

        // Only need fvar patch param for bspline or box spline patches
        if (_geometricShader->GetFvarPatchType() == 
            HdSt_GeometricShader::FvarPatchType::PATCH_BSPLINE ||
            _geometricShader->GetFvarPatchType() ==
            HdSt_GeometricShader::FvarPatchType::PATCH_BOXSPLINETRIANGLE) {
            _EmitAccessor(accessors, name,
                _metaData->fvarPatchParamBindings[i].dataType, binding,
                "GetPrimitiveIndex() + localIndex");
        }
    }

    _genTCS << accessors.str();
    _genTES << accessors.str();
    _genGS << accessors.str();
    _genPTCS << accessors.str();
    _genPTVS << accessors.str();
    _genFS << accessors.str();
}

void
HdSt_CodeGen::_GenerateVertexAndFaceVaryingPrimvar()
{
    if (_geometricShader->IsFrustumCullingPass()) {
        return;
    }

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

      void ProcessPrimvarsIn() {
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

      void ProcessPrimvarsOut(int index) {
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

    std::stringstream accessorsVS, accessorsTCS, accessorsTES,
        accessorsPTCS, accessorsPTVS, accessorsGS, accessorsFS;

    HioGlslfxResourceLayout::MemberVector interstagePrimvar;

    // vertex 
    TF_FOR_ALL (it, _metaData->vertexData) {
        HdStBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;

        // future work:
        // with ARB_enhanced_layouts extention, it's possible
        // to use "component" qualifier to declare offsetted primvars
        // in interleaved buffer.
        _EmitDeclaration(&_resAttrib, name, dataType, binding);

        interstagePrimvar.emplace_back(_GetPackedType(dataType, false), name);

        // primvar accessors
        _EmitAccessor(accessorsVS, name, dataType, binding);

        _EmitStructAccessor(accessorsTCS, _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "gl_InvocationID");
        _EmitStructAccessor(accessorsTES, _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsGS,  _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsFS,  _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1);

        // Access PTCS vertex primvar from input attributes.
        _EmitStageAccessor(accessorsPTCS, name,
            name.GetString() + "[localIndex]", dataType);

        // Access PTVS vertex primvar from input attributes.
        _EmitStageAccessor(accessorsPTVS, name,
            name.GetString() + "[localIndex]", dataType);

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

        _procPTVSOut << "  outPrimvars." << name
                     << " = InterpolatePrimvar("
                     << "HdGet_" << name << "(i0), "
                     << "HdGet_" << name << "(i1), "
                     << "HdGet_" << name << "(i2), "
                     << "HdGet_" << name << "(i3), basis, uv);\n";
    }

    /*
      // --------- varying data declaration (VS) ----------------
      layout (std430, binding=?) buffer buffer0 {
          vec3 displayColor[];
      };

      vec3 HdGet_displayColor(int localIndex) {
        int index =  GetDrawingCoord().varyingCoord + int(hd_VertexID) - 
            GetBaseVertexOffset();
        return vec3(displayColor[index]);
      }
      vec3 HdGet_displayColor() { return HdGet_displayColor(0); }

      out Primvars {
          vec3 displayColor;
      } outPrimvars;

      void ProcessPrimvarsIn() {
          outPrimvars.displayColor = HdGet_displayColor();
      }

      // --------- fragment stage plumbing -------
      in Primvars {
          vec3 displayColor;
      } inPrimvars;
    */

    HdSt_ResourceBinder::MetaData::BindingDeclaration const &
            indexBufferBinding = _metaData->indexBufferBinding;
    if (!indexBufferBinding.name.IsEmpty()) {
        _EmitDeclaration(&_resPTCS,
                         indexBufferBinding.name,
                         indexBufferBinding.dataType,
                         indexBufferBinding.binding);
        _EmitDeclaration(&_resPTVS,
                         indexBufferBinding.name,
                         indexBufferBinding.dataType,
                         indexBufferBinding.binding);

        _EmitBufferAccessor(accessorsPTCS,
                            indexBufferBinding.name,
                            indexBufferBinding.dataType,
            "patch_id * VERTEX_CONTROL_POINTS_PER_PATCH + localIndex");
        _EmitBufferAccessor(accessorsPTVS,
                            indexBufferBinding.name,
                            indexBufferBinding.dataType,
            "patch_id * VERTEX_CONTROL_POINTS_PER_PATCH + localIndex");
    }

    TF_FOR_ALL (it, _metaData->varyingData) {
        HdStBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;

        _EmitDeclaration(&_resAttrib, name, dataType, binding);

        interstagePrimvar.emplace_back(_GetPackedType(dataType, false), name);

        // primvar accessors
        _EmitBufferAccessor(accessorsVS, name, dataType,
            "GetDrawingCoord().varyingCoord + int(hd_VertexID) - GetBaseVertexOffset()");
        
        _EmitStructAccessor(accessorsTCS, _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "gl_InvocationID");
        _EmitStructAccessor(accessorsTES, _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsGS,  _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsFS,  _tokens->inPrimvars,
                            name, dataType, /*arraySize=*/1);

        // Access PTCS varying primvar from varying data buffer.
        _EmitBufferAccessor(accessorsPTCS, name, dataType,
            "GetDrawingCoord().varyingCoord + HdGet_indices(localIndex)");

        // Access PTVS varying primvar from varying data buffer.
        _EmitBufferAccessor(accessorsPTVS, name, dataType,
            "GetDrawingCoord().varyingCoord + HdGet_indices(localIndex)");
        
        // interstage plumbing
        _procVS << "  outPrimvars." << name
                << " = " << "HdGet_" << name << "();\n";
        _procTCS << "  outPrimvars[gl_InvocationID]." << name
                 << " = inPrimvars[gl_InvocationID]." << name << ";\n";
        _procTES << "  outPrimvars." << name
                 << " = InterpolatePrimvar("
                 << "inPrimvars[i0]." << name 
                 << ", inPrimvars[i1]." << name 
                 << ", inPrimvars[i2]." << name 
                 << ", inPrimvars[i3]." << name 
                 << ", basis, uv);\n";
        _procGS  << "  outPrimvars." << name
                 << " = inPrimvars[index]." << name << ";\n";

        _procPTVSOut << "  outPrimvars." << name
                     << " = InterpolatePrimvar("
                     << "HdGet_" << name << "(i0), "
                     << "HdGet_" << name << "(i1), "
                     << "HdGet_" << name << "(i2), "
                     << "HdGet_" << name << "(i3), basis, uv);\n";
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

      void ProcessPrimvarsOut(int index) {
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
    HioGlslfxResourceLayout::MemberVector interstagePrimvarFVar;

    // FVar primvars are emitted by GS or FS
    TF_FOR_ALL (it, _metaData->fvarData) {
        HdStBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;
        const int channel = it->second.channel;

        if (_hasGS) {
            _EmitDeclaration(&_resMaterial, name, dataType, binding);

            interstagePrimvarFVar.emplace_back(
                _GetPackedType(dataType, false), name);

            // primvar accessors (only in GS and FS)
            _EmitFVarAccessor(_hasGS, accessorsGS, name, dataType, binding,
                              _geometricShader->GetPrimitiveType(),
                              _geometricShader->GetFvarPatchType(),
                              channel);

            _EmitStructAccessor(accessorsFS, _tokens->inPrimvars,
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
            _EmitDeclaration(&_resMaterial, name, dataType, binding);

            _EmitFVarAccessor(_hasGS,
                              accessorsFS, name, dataType, binding,
                              _geometricShader->GetPrimitiveType(),
                              _geometricShader->GetFvarPatchType(),
                              channel);

            _EmitFVarAccessor(false,
                              accessorsPTCS, name, dataType, binding,
                              _geometricShader->GetPrimitiveType(),
                              _geometricShader->GetFvarPatchType(),
                              channel);

            _EmitFVarAccessor(false,
                              accessorsPTVS, name, dataType, binding,
                              _geometricShader->GetPrimitiveType(),
                              _geometricShader->GetFvarPatchType(),
                              channel);
        }
    }

    if (!interstagePrimvar.empty()) {
        // VS out
        _AddInterstageBlockElement(
            &_resVS, HioGlslfxResourceLayout::InOut::STAGE_OUT,
            _tokens->PrimvarData, _tokens->outPrimvars, interstagePrimvar);

        // TCS in/out
        _AddInterstageBlockElement(
            &_resTCS, HioGlslfxResourceLayout::InOut::STAGE_IN,
            _tokens->PrimvarData, _tokens->inPrimvars, interstagePrimvar,
            _tokens->gl_MaxPatchVertices);
        _AddInterstageBlockElement(
            &_resTCS, HioGlslfxResourceLayout::InOut::STAGE_OUT,
            _tokens->PrimvarData, _tokens->outPrimvars, interstagePrimvar,
            _tokens->HD_NUM_PATCH_EVAL_VERTS);

        // TES in/out
        _AddInterstageBlockElement(
            &_resTES, HioGlslfxResourceLayout::InOut::STAGE_IN,
            _tokens->PrimvarData, _tokens->inPrimvars, interstagePrimvar,
            _tokens->gl_MaxPatchVertices);
        _AddInterstageBlockElement(
            &_resTES, HioGlslfxResourceLayout::InOut::STAGE_OUT,
            _tokens->PrimvarData, _tokens->outPrimvars, interstagePrimvar);

        // GS in
        _AddInterstageBlockElement(
            &_resGS, HioGlslfxResourceLayout::InOut::STAGE_IN,
            _tokens->PrimvarData, _tokens->inPrimvars, interstagePrimvar,
            _tokens->HD_NUM_PRIMITIVE_VERTS);
    }

    if (!interstagePrimvar.empty() || !interstagePrimvarFVar.empty()) {

        // Include FVar primvar for these shader stages.
        interstagePrimvar.insert(interstagePrimvar.end(),
                                 interstagePrimvarFVar.begin(),
                                 interstagePrimvarFVar.end());

        // PTVS out
        _AddInterstageBlockElement(
            &_resPTVS, HioGlslfxResourceLayout::InOut::STAGE_OUT,
            _tokens->PrimvarData, _tokens->outPrimvars, interstagePrimvar);

        // GS out
        _AddInterstageBlockElement(
            &_resGS, HioGlslfxResourceLayout::InOut::STAGE_OUT,
            _tokens->PrimvarData, _tokens->outPrimvars, interstagePrimvar);

        // FS in
        _AddInterstageBlockElement(
            &_resFS, HioGlslfxResourceLayout::InOut::STAGE_IN,
            _tokens->PrimvarData, _tokens->inPrimvars, interstagePrimvar);
    }

    _genVS    << accessorsVS.str();
    _genGS    << accessorsGS.str();
    _genFS    << accessorsFS.str();
    _genTCS   << accessorsTCS.str();
    _genTES   << accessorsTES.str();
    _genPTCS  << accessorsPTCS.str();
    _genPTVS  << accessorsPTVS.str();

    // ---------
    _genFS << "FORWARD_DECL(vec4 GetPatchCoord(int index));\n";
    _genGS << "FORWARD_DECL(vec4 GetPatchCoord(int localIndex));\n";
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

    std::stringstream accessors;

    TfToken typeName("ShaderData");
    TfToken varName("shaderData");

    // for shader parameters, we create declarations and accessors separetely.
    TF_FOR_ALL (it, _metaData->shaderData) {
        HdStBinding binding = it->first;

        _genDecl << "struct " << typeName << " {\n";

        TF_FOR_ALL (dbIt, it->second.entries) {
            _genDecl << "  " << _GetPackedType(_ConvertBoolType(dbIt->dataType), false)
                     << " " << dbIt->name
                     << ";\n";
        }

        _genDecl << "};\n";

        // for array delaration, SSBO and bindless uniform can use [].
        // UBO requires the size [N].
        // XXX: [1] is a hack to cheat driver not telling the actual size.
        //      may not work some GPUs.
        // XXX: we only have 1 shaderData entry (interleaved).
        int arraySize = (binding.GetType() == HdStBinding::UBO) ? 1 : 0;
        _EmitDeclaration(&_resCommon, varName, typeName, binding, arraySize);

        break;
    }

    // Non-field redirect accessors.
    TF_FOR_ALL (it, _metaData->shaderParameterBinding) {

        // adjust datatype
        std::string swizzle = _GetSwizzleString(it->second.dataType,
                                                it->second.swizzle);
        std::string fallbackSwizzle =
            _GetFallbackScalarSwizzleString(it->second.dataType,
                                            it->second.name);

        HdStBinding const &binding = it->first;
        HdStBinding::Type const bindingType = binding.GetType();

        if (bindingType == HdStBinding::FALLBACK) {

            // vec4 HdGet_name(int localIndex)
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "(int localIndex) {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return "
                << _GetPackedTypeAccessor(it->second.dataType, false)
                << "(shaderData[shaderCoord]."
                << it->second.name << HdSt_ResourceBindingSuffixTokens->fallback
                << fallbackSwizzle
                << ");\n"
                << "}\n";

            // vec4 HdGet_name()
            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name
                << "() { return HdGet_" << it->second.name << "(0); }\n";

            // float HdGetScalar_name()
            _EmitScalarAccessor(
                accessors, it->second.name, it->second.dataType);

        } else if (bindingType == HdStBinding::BINDLESS_TEXTURE_2D) {

            _EmitTextureAccessors(
                accessors, it->second, swizzle, fallbackSwizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ true,
                /* isBindless = */ true,
                bindlessTextureEnabled);

        } else if (bindingType == HdStBinding::BINDLESS_ARRAY_OF_TEXTURE_2D) {

            // Handle special case for shadow textures.
            bool const isShadowTexture = 
                (it->second.name == HdStTokens->shadowCompareTextures);

            _EmitTextureAccessors(
                accessors, it->second, swizzle, fallbackSwizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ !isShadowTexture,
                /* isBindless = */ true,
                bindlessTextureEnabled,
                /* isArray = */ true,
                /* isShadowSampler = */ isShadowTexture);

        } else if (bindingType == HdStBinding::TEXTURE_2D) {

            _AddTextureElement(&_resTextures,
                               it->second.name, 2,
                               binding.GetTextureUnit());

            _EmitTextureAccessors(
                accessors, it->second, swizzle, fallbackSwizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ true,
                /* isBindless = */ false,
                bindlessTextureEnabled);

        } else if (bindingType == HdStBinding::ARRAY_OF_TEXTURE_2D) {       

            // Handle special case for shadow textures.
            bool const isShadowTexture = 
                (it->second.name == HdStTokens->shadowCompareTextures);

            _AddArrayOfTextureElement(&_resTextures,
                                      it->second.name, 2,
                                      binding.GetTextureUnit(),
                                      HioFormatFloat32Vec4,
                                      isShadowTexture
                                        ? TextureType::SHADOW_TEXTURE
                                        : TextureType::TEXTURE,
                                      it->second.arrayOfTexturesSize);

            _EmitTextureAccessors(
                accessors, it->second, swizzle, fallbackSwizzle,
                /* dim = */ 2,
                /* hasTextureTransform = */ false,
                /* hasTextureScaleAndBias = */ !isShadowTexture,
                /* isBindless = */ false,
                bindlessTextureEnabled,
                /* isArray = */ true,
                /* isShadowSampler = */ isShadowTexture);

        } else if (bindingType == HdStBinding::BINDLESS_TEXTURE_FIELD) {

            _EmitTextureAccessors(
                accessors, it->second, swizzle, fallbackSwizzle,
                /* dim = */ 3,
                /* hasTextureTransform = */ true,
                /* hasTextureScaleAndBias = */ false,
                /* isBindless = */ true,
                bindlessTextureEnabled);

        } else if (bindingType == HdStBinding::TEXTURE_FIELD) {

            _AddTextureElement(&_resTextures,
                               it->second.name, 3,
                               binding.GetTextureUnit());

            _EmitTextureAccessors(
                accessors, it->second, swizzle, fallbackSwizzle,
                /* dim = */ 3,
                /* hasTextureTransform = */ true,
                /* hasTextureScaleAndBias = */ false,
                /* isBindless = */ false,
                bindlessTextureEnabled);

        } else if (bindingType == HdStBinding::BINDLESS_TEXTURE_UDIM_ARRAY) {

            accessors 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->storm << "_" 
                << HdStTokens->scale << "\n"
                << "vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
                << HdStTokens->scale << "();\n"
                << "#endif\n"
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
                << HdStTokens->bias << "\n"
                << "vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
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
                << " HdGet_" << it->second.name << "(vec2 coord)" << " {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord;\n"
                << "  uvec2 handle = shaderData[shaderCoord]."
                << it->second.name
                << HdSt_ResourceBindingSuffixTokens->layout << ";\n"
                << "  vec3 c = hd_sample_udim(coord);\n"
                << "  c.z = "
                << "texelFetch(sampler1D(handle), int(c.z), 0).x - 1;\n"
                << "  vec4 ret = vec4(0, 0, 0, 0);\n"
                << "  if (c.z >= -0.5) {\n"
                << "    uvec2 handleTexels = shaderData[shaderCoord]."
                << it->second.name << ";\n"
                << "    ret = texture(sampler2DArray(handleTexels), c);\n"
                << "  }\n";
                
            if (it->second.processTextureFallbackValue) {
                accessors
                    << "  if (!bool(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->valid
                    << ")) {\n"
                    << "    return ("
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->fallback
                    << fallbackSwizzle << ")\n"
                    << "#ifdef HD_HAS_" << it->second.name << "_"
                    << HdStTokens->storm << "_"
                    << HdStTokens->scale << "\n"
                    << "    * HdGet_" << it->second.name << "_" 
                    << HdStTokens->storm << "_"
                    << HdStTokens->scale << "()" << swizzle << "\n"
                    << "#endif\n" 
                    << "#ifdef HD_HAS_" << it->second.name << "_" 
                    << HdStTokens->storm << "_"
                    << HdStTokens->bias << "\n"
                    << "    + HdGet_" << it->second.name << "_" 
                    << HdStTokens->storm << "_"
                    << HdStTokens->bias  << "()" << swizzle << "\n"
                    << "#endif\n"
                    << "    );\n  }\n";
            }
            
            accessors
                << "  return (ret\n"
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
                << HdStTokens->scale << "\n"
                << "    * HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
                << HdStTokens->scale << "()\n"
                << "#endif\n" 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->storm << "_" 
                << HdStTokens->bias << "\n"
                << "    + HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_" 
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

            // vec4 HdGet_name() { return HdGet_name(HdGetCoord_name()); }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "() { return HdGet_" << it->second.name << "("
                << "HdGetCoord_" << it->second.name << "()); }\n";

            // vec4 HdGet_name(int localIndex) { return HdGet_name(HdGetCoord_name()); }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "(int localIndex) { return HdGet_" << it->second.name << "("
                << "HdGetCoord_" << it->second.name << "());\n}\n";

            // float HdGetScalar_name()
            _EmitScalarAccessor(
                accessors, it->second.name, it->second.dataType);

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }      

        } else if (bindingType == HdStBinding::TEXTURE_UDIM_ARRAY) {

            accessors 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
                << HdStTokens->scale << "\n"
                << "FORWARD_DECL(vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
                << HdStTokens->scale << "());\n"
                << "#endif\n"
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->storm << "_"
                << HdStTokens->bias << "\n"
                << "FORWARD_DECL(vec4 HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_" 
                << HdStTokens->bias << "());\n"
                << "#endif\n";
                
            _AddTextureElement(&_resTextures,
                               it->second.name, 2,
                               binding.GetTextureUnit(),
                               HioFormatFloat32Vec4,
                               TextureType::ARRAY_TEXTURE);

            // vec4 HdGet_name(vec2 coord) { vec3 c = hd_sample_udim(coord);
            // c.z = HgiTexelFetch_name(int(c.z), 0).x - 1;
            // vec4 ret = vec4(0, 0, 0, 0);
            // if (c.z >= -0.5) { ret = HgiGet_name(c); }
            // return (ret
            // #ifdef HD_HAS_name_scale
            //   * HdGet_name_scale()
            // #endif
            // #ifdef HD_HAS_name_bias
            //   + HdGet_name_bias()
            // #endif // ).xyz; }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "(vec2 coord) {\n vec3 c = hd_sample_udim(coord);\n"
                << "  c.z = HgiTexelFetch_"
                << it->second.name << HdSt_ResourceBindingSuffixTokens->layout
                << "(int(c.z)).x - 1;\n"
                << "  vec4 ret = vec4(0, 0, 0, 0);\n"
                << "  if (c.z >= -0.5) { ret = HgiGet_"
                << it->second.name << "(c); }\n";
            
            if (it->second.processTextureFallbackValue) {
                accessors
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord;\n"
                    << "  if (!bool(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->valid
                    << ")) {\n"
                    << "    return ("
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->fallback
                    << fallbackSwizzle << ")\n"
                    << "#ifdef HD_HAS_" << it->second.name << "_"
                    << HdStTokens->storm << "_" << HdStTokens->scale << "\n"
                    << "    * HdGet_" << it->second.name << "_" 
                    << HdStTokens->storm << "_" << HdStTokens->scale << "()" 
                    << swizzle << "\n"
                    << "#endif\n" 
                    << "#ifdef HD_HAS_" << it->second.name << "_" 
                    << HdStTokens->storm << "_" << HdStTokens->bias << "\n"
                    << "    + HdGet_" << it->second.name << "_" 
                    << HdStTokens->storm << "_" << HdStTokens->bias  << "()" 
                    << swizzle << "\n"
                    << "#endif\n"
                    << "    );\n  }\n";
            }

            accessors
                << "  return (ret\n"
                << "#ifdef HD_HAS_" << it->second.name << "_"
                << HdStTokens->storm << "_" << HdStTokens->scale << "\n"
                << "    * HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_" << HdStTokens->scale << "()\n"
                << "#endif\n" 
                << "#ifdef HD_HAS_" << it->second.name << "_" 
                << HdStTokens->storm << "_" << HdStTokens->bias << "\n"
                << "    + HdGet_" << it->second.name << "_" 
                << HdStTokens->storm << "_" << HdStTokens->bias  << "()\n"
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

            // vec4 HdGet_name(int localIndex) { return HdGet_name(HdGetCoord_name()); }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "(int localIndex) { return HdGet_" << it->second.name << "("
                << "HdGetCoord_" << it->second.name << "());\n}\n";

            // float HdGetScalar_name()
            _EmitScalarAccessor(
                accessors, it->second.name, it->second.dataType);

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }

        } else if (bindingType == HdStBinding::TEXTURE_UDIM_LAYOUT) {

            _AddTextureElement(&_resTextures,
                               it->second.name, 1,
                               binding.GetTextureUnit());

        } else if (bindingType == HdStBinding::BINDLESS_TEXTURE_PTEX_TEXEL) {
            
            if (it->second.processTextureFallbackValue) {
                accessors
                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(int localIndex) {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  if (bool(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->valid
                    << ")) {\n"
                    << "    return " 
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "sampler2DArray(shaderData[shaderCoord]."
                    << it->second.name << "),"
                    << "usampler1DArray(shaderData[shaderCoord]."
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->layout
                    <<"), "
                    << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                    << "  } else {\n"
                    << "    return ("
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(shaderData[shaderCoord]."
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->fallback
                    << fallbackSwizzle << "))" << swizzle << ";\n" << "  }\n}\n"

                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  if (bool(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->valid
                    << ")) {\n"
                    << "    return " 
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "sampler2DArray(shaderData[shaderCoord]."
                    << it->second.name << "),"
                    << "usampler1DArray(shaderData[shaderCoord]."
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->layout
                    << "), "
                    << "patchCoord)" << swizzle << ");\n"
                    << "  } else {\n"
                    << "    return ("
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(shaderData[shaderCoord]."
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->fallback
                    << fallbackSwizzle << "))" << swizzle << ";\n"
                    << "  }\n}\n";
            } else {
                accessors
                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(int localIndex) {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  return " 
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "sampler2DArray(shaderData[shaderCoord]."
                    << it->second.name << "),"
                    << "usampler1DArray(shaderData[shaderCoord]."
                    << it->second.name 
                    << HdSt_ResourceBindingSuffixTokens->layout
                    <<"), "
                    << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                    << "}\n"

                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  return " 
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "sampler2DArray(shaderData[shaderCoord]."
                    << it->second.name << "),"
                    << "usampler1DArray(shaderData[shaderCoord]."
                    << it->second.name 
                    << HdSt_ResourceBindingSuffixTokens->layout
                    << "), "
                    << "patchCoord)" << swizzle << ");\n"
                    << "}\n";
            }

            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "()"
                << "{ return HdGet_" << it->second.name << "(0); }\n";

            // float HdGetScalar_name()
            _EmitScalarAccessor(
                accessors, it->second.name, it->second.dataType);

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }     

        } else if (bindingType == HdStBinding::TEXTURE_PTEX_TEXEL) {

            _AddTextureElement(&_resTextures,
                               it->second.name, 2,
                               binding.GetTextureUnit(),
                               HioFormatFloat32Vec4,
                               TextureType::ARRAY_TEXTURE);
            if (it->second.processTextureFallbackValue) {
                accessors
                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(int localIndex) {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  if (bool(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->valid
                    << ")) {\n"
                    << "    return "
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "HgiGetSampler_" << it->second.name << "(), "
                    << "HgiGetSampler_"
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->layout
                    << "(), "
                    << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                    << "  } else {\n"
                    << "    return ("
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(shaderData[shaderCoord]."
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->fallback
                    << fallbackSwizzle << "))" << swizzle << ";\n" << "  }\n}\n"
                                    
                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  if (bool(shaderData[shaderCoord]." << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->valid
                    << ")) {\n"
                    << "    return "
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "HgiGetSampler_" << it->second.name << "(), "
                    << "HgiGetSampler_"
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->layout
                    << "(), "
                    << "patchCoord)" << swizzle << ");\n"
                    << "  } else {\n"
                    << "    return ("
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(shaderData[shaderCoord]."
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->fallback
                    << fallbackSwizzle << "))" << swizzle << ";\n"
                    << "  }\n}\n";
            } else {
                accessors
                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(int localIndex) {\n"
                    << "  return "
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "HgiGetSampler_" << it->second.name << "(), "
                    << "HgiGetSampler_"
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->layout
                    << "(), "
                    << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                    << "}\n"
                
                    << _GetUnpackedType(it->second.dataType, false)
                    << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                    << "  return "
                    << _GetPackedTypeAccessor(it->second.dataType, false)
                    << "(PtexTextureLookup("
                    << "HgiGetSampler_" << it->second.name << "(), "
                    << "HgiGetSampler_"
                    << it->second.name
                    << HdSt_ResourceBindingSuffixTokens->layout
                    << "(), "
                    << "patchCoord)" << swizzle << ");\n"
                    << "}\n";
            }

            accessors
                << _GetUnpackedType(it->second.dataType, false)
                << " HdGet_" << it->second.name << "()"
                << "{ return HdGet_" << it->second.name << "(0); }\n";

            // float HdGetScalar_name()
            _EmitScalarAccessor(
                accessors, it->second.name, it->second.dataType);

            // Emit pre-multiplication by alpha indicator
            if (it->second.isPremultiplied) {
                accessors 
                    << "#define " << it->second.name << "_IS_PREMULTIPLIED 1\n";
            }    

        } else if (bindingType == HdStBinding::BINDLESS_TEXTURE_PTEX_LAYOUT) {

            //_genAccessors << _GetUnpackedType(it->second.dataType) << "(0)";

        } else if (bindingType == HdStBinding::TEXTURE_PTEX_LAYOUT) {

            _AddTextureElement(&_resTextures,
                               it->second.name, 1,
                               binding.GetTextureUnit(),
                               HioFormatUInt16,
                               TextureType::ARRAY_TEXTURE);

        } else if (bindingType == HdStBinding::PRIMVAR_REDIRECT) {

            // Create an HdGet_INPUTNAME for the shader to access a primvar
            // for which a HdGet_PRIMVARNAME was already generated earlier.
            
            // XXX: shader and primvar name collisions are a problem!
            // (see, e.g., HYD-1800).
            if (it->second.name == it->second.inPrimvars[0]) {
                // Avoid the following:
                // If INPUTNAME and PRIMVARNAME are the same and the
                // primvar exists, we would generate two functions
                // both called HdGet_PRIMVAR, one to read the primvar
                // (based on _metaData->constantData) and one for the
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
                << fallbackSwizzle <<  ");\n"
                << "#endif\n"
                << "\n}\n"
                << "#define HD_HAS_" << it->second.name << " 1\n";
            
            // Emit scalar accessors to support shading languages like MSL which
            // do not support swizzle operators on scalar values.
            if (_GetNumComponents(it->second.dataType) <= 4) {
                accessors
                    << _GetFlatType(it->second.dataType) << " HdGetScalar_"
                    << it->second.name << "()"
                    << " { return HdGet_" << it->second.name << "()"
                    << _GetFlatTypeSwizzleString(it->second.dataType)
                    << "; }\n";
            }

            if (it->second.name == it->second.inPrimvars[0]) {
                accessors
                    << "#endif\n";
            }

        } else if (bindingType == HdStBinding::TRANSFORM_2D) {

            // Forward declare rotation, scale, and translation
            accessors 
                << "FORWARD_DECL(float HdGet_" << it->second.name << "_" 
                << HdStTokens->rotation  << "());\n"
                << "FORWARD_DECL(vec2 HdGet_" << it->second.name << "_" 
                << HdStTokens->scale  << "());\n"
                << "FORWARD_DECL(vec2 HdGet_" << it->second.name << "_" 
                << HdStTokens->translation  << "());\n";

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
                << HdSt_ResourceBindingSuffixTokens->fallback << fallbackSwizzle
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

    TF_FOR_ALL (it, _metaData->shaderParameterBinding) {
        const HdStBinding::Type bindingType = it->first.GetType();

        if ( bindingType == HdStBinding::TEXTURE_FIELD ||
             bindingType == HdStBinding::BINDLESS_TEXTURE_FIELD) {

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
    TF_FOR_ALL (it, _metaData->shaderParameterBinding) {
        HdStBinding::Type bindingType = it->first.GetType();

        if (bindingType == HdStBinding::FIELD_REDIRECT) {

            // adjust datatype
            std::string swizzle = _GetSwizzleString(it->second.dataType);
            std::string fallbackSwizzle =
                _GetFallbackScalarSwizzleString(it->second.dataType,
                                                it->second.name);

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
                << fallbackSwizzle << ");\n"
                << "#endif\n"
                << "\n}\n";
        }
    }

    _genGS << accessors.str();
    _genFS << accessors.str();
    _genPTCS << accessors.str();
    _genPTVS << accessors.str();
}

void
HdSt_CodeGen::_GenerateTopologyVisibilityParameters()
{
    std::stringstream declarations;
    std::stringstream accessors;
    TF_FOR_ALL (it, _metaData->topologyVisibilityData) {
        // See note in _GenerateConstantPrimvar re: padding.
        HdStBinding binding = it->first;
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

        _EmitDeclaration(&_resCommon, varName, typeName, binding,
                         /*arraySize=*/1);
    }

    _genDecl << declarations.str();
    _genAccessors << accessors.str();
}

std::string
HdSt_CodeGen::_GetFallbackScalarSwizzleString(TfToken const &returnType,
                                              TfToken const &paramName)
{
    if (!_IsScalarType(returnType)) {
        return "";
    }

    // TODO: More efficient way of either specifying this at a higher level
    // or calculating it in codeGen
    TfToken fallbackParamName(paramName.GetString() +
        HdSt_ResourceBindingSuffixTokens->fallback.GetString());
    TF_FOR_ALL (it, _metaData->shaderData) {
        TF_FOR_ALL (dbIt, it->second.entries) {
            if (dbIt->name == fallbackParamName) {
                if (!_IsScalarType(dbIt->dataType)) {
                    return ".x";
                }
                return "";
            }
        }
    }

    return "";
}

PXR_NAMESPACE_CLOSE_SCOPE

