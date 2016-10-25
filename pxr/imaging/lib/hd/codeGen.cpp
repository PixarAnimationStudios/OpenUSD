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
#include "pxr/imaging/hd/codeGen.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/drawBatch.h"
#include "pxr/imaging/hd/geometricShader.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

#include <sstream>

#include <opensubdiv/osd/glslPatchShaderSource.h>

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_float, "float"))
    ((_int, "int"))
    (hd_vec3)
    (hd_vec3_get)
    (hd_ivec3)
    (hd_ivec3_get)
    (hd_dvec3)
    (hd_dvec3_get)
    (inPrimVars)
    (ivec2)
    (ivec3)
    (ivec4)
    (outPrimVars)
    (vec2)
    (vec3)
    (vec4)
    (dvec3)
    ((ptexTextureSampler, "ptexTextureSampler"))
    (isamplerBuffer)
    (samplerBuffer)
);

Hd_CodeGen::Hd_CodeGen(Hd_GeometricShaderPtr const &geometricShader,
                       HdShaderSharedPtrVector const &shaders)
    : _geometricShader(geometricShader), _shaders(shaders)
{
    TF_VERIFY(geometricShader);
}

Hd_CodeGen::ID
Hd_CodeGen::ComputeHash() const
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    ID hash = _geometricShader->ComputeHash();
    boost::hash_combine(hash, _metaData.ComputeHash());
    boost::hash_combine(hash, HdShader::ComputeHash(_shaders));

    return hash;
}

static
std::string
_GetPtexTextureShaderSource()
{
    static std::string source =
        GlfGLSLFX(HdPackagePtexTextureShader()).GetSource(
            _tokens->ptexTextureSampler);
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
           "ivec3 hd_ivec3_get(hd_ivec3 v) { return ivec3(v.x, v.y, v.z); }\n"
           "ivec3 hd_ivec3_get(ivec3 v)    { return v; }\n"
           "vec3  hd_vec3_get(hd_vec3 v)   { return vec3(v.x, v.y, v.z); }\n"
           "vec3  hd_vec3_get(vec3 v)      { return v; }\n"
           "dvec3 hd_dvec3_get(hd_dvec3 v) { return dvec3(v.x, v.y, v.z); }\n"
           "dvec3 hd_dvec3_get(dvec3 v)    { return v; }\n"
        // helper functions for 410 specification
        // applying a swizzle operator on int and float is not allowed in 410.
           "int hd_int_get(int v)          { return v; }\n"
           "int hd_int_get(ivec2 v)        { return v.x; }\n"
           "int hd_int_get(ivec3 v)        { return v.x; }\n"
           "int hd_int_get(ivec4 v)        { return v.x; }\n";
}

static TfToken const &
_GetPackedType(TfToken const &token)
{
    if (token == _tokens->ivec3) {
        return _tokens->hd_ivec3;
    } else if (token == _tokens->vec3) {
        return _tokens->hd_vec3;
    } else if (token == _tokens->dvec3) {
        return _tokens->hd_dvec3;
    }
    return token;
}

static TfToken const &
_GetPackedTypeAccessor(TfToken const &token)
{
    if (token == _tokens->ivec3) {
        return _tokens->hd_ivec3_get;
    } else if (token == _tokens->vec3) {
        return _tokens->hd_vec3_get;
    } else if (token == _tokens->dvec3) {
        return _tokens->hd_dvec3_get;
    }
    return token;
}

static TfToken const &
_GetSamplerBufferType(TfToken const &token)
{
    if (token == _tokens->_int or
        token == _tokens->ivec2 or
        token == _tokens->ivec3 or
        token == _tokens->ivec4) {
        return _tokens->isamplerBuffer;
    } else {
        return _tokens->samplerBuffer;
    }
}

namespace {
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
        HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
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
        case HdBinding::TBO:
        case HdBinding::BINDLESS_UNIFORM:
        case HdBinding::TEXTURE_2D:
        case HdBinding::BINDLESS_TEXTURE_2D:
        case HdBinding::TEXTURE_PTEX_TEXEL:
        case HdBinding::TEXTURE_PTEX_LAYOUT:
            if (caps.explicitUniformLocation) {
                out << "layout (location = " << location << ") ";
            }
            break;
        case HdBinding::SSBO:
            out << "layout (std430, binding = " << location << ") ";
            break;
        case HdBinding::UBO:
            if (caps.shadingLanguage420pack) {
                out << "layout (std140, binding = " << location << ") ";
            } else {
                out << "layout (std140)";
            }
            break;
        default:
            break;
        }
        return out;
    }
}

HdGLSLProgramSharedPtr
Hd_CodeGen::Compile()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // create GLSL program.

    HdGLSLProgramSharedPtr glslProgram(
        new HdGLSLProgram(HdTokens->drawingShader));

    // initialize autogen source buckets
    _genCommon.str(""); _genVS.str(""); _genTCS.str(""); _genTES.str("");
    _genGS.str(""); _genFS.str("");
    _procVS.str(""); _procTCS.str(""), _procTES.str(""), _procGS.str("");

    // GLSL version.
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();
    _genCommon << "#version " << caps.glslVersion << "\n";

    if (caps.bindlessBufferEnabled) {
        _genCommon << "#extension GL_NV_shader_buffer_load : require\n"
                   << "#extension GL_NV_gpu_shader5 : require\n";
    }
    if (caps.bindlessTextureEnabled) {
        _genCommon << "#extension GL_ARB_bindless_texture : require\n";
    }
    if (caps.glslVersion < 430 and caps.explicitUniformLocation) {
        _genCommon << "#extension GL_ARB_explicit_uniform_location : require\n";
    }
    if (caps.glslVersion < 420 and caps.shadingLanguage420pack) {
        _genCommon << "#extension GL_ARB_shading_language_420pack : require\n";
    }

    // Used in glslfx files to determine if it is using new/old
    // imaging system. It can also be used as API guards when
    // we need new versions of Hydra shading. 
    _genCommon << "#define HD_SHADER_API " << HD_SHADER_API << "\n";

    // XXX: this is a hacky workaround for experimental support of GL 3.3
    //      the double is used in hd_dvec3 akin, so we are likely able to
    //      refactor that helper functions.
    if (caps.glslVersion < 400) {
        _genCommon << "#define double float\n"
                   << "#define dvec2 vec2\n"
                   << "#define dvec3 vec3\n"
                   << "#define dvec4 vec4\n"
                   << "#define dmat4 mat4\n";
    }

    // XXX: this macro is still used in GlobalUniform.
    if (HdVtBufferSource::GetDefaultMatrixType() == GL_FLOAT) {
        _genCommon << "#define MAT4 mat4\n";
    } else {
        _genCommon << "#define MAT4 dmat4\n";
    }
    // a trick to tightly pack vec3 into SSBO/UBO.
    _genCommon << _GetPackedTypeDefinitions();

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
        // note: _constantData has been sorted by offset in Hd_ResourceBinder.
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
            declarations << "  " << dbIt->dataType
                         << " " << dbIt->name;
            if (dbIt->arraySize > 1) {
                _genCommon << "#define HD_NUM_" << dbIt->name
                           << " " << dbIt->arraySize << "\n";
                declarations << "[" << dbIt->arraySize << "]";
            }
            declarations <<  ";\n";

            _EmitStructAccessor(accessors, varName, 
                                dbIt->name, dbIt->dataType, dbIt->arraySize,
                                NULL);
        }

        declarations << "};\n";
        _EmitDeclaration(declarations, varName, typeName, binding);
    }
    _genCommon << declarations.str()
               << accessors.str();

    // XXX: this is a too mesh specific inference. need a better way
    //
    // find out the primitive parameterization type
    int primType = PRIM_OTHER;
    if (_metaData.primitiveParamBinding.dataType == _tokens->_int) {
        primType = PRIM_TRI;
    } else if (_metaData.primitiveParamBinding.dataType == _tokens->ivec2) {
        primType = PRIM_COARSE_QUAD;
    } else if (_metaData.primitiveParamBinding.dataType == _tokens->ivec3) {
        primType = PRIM_REFINED_QUAD;
    } else if (_metaData.primitiveParamBinding.dataType == _tokens->ivec4) {
        primType = PRIM_PATCH;
    }

    // HD_NUM_PATCH_VERTS, HD_NUM_PRIMTIIVE_VERTS
    GLenum glPrimitiveMode = _geometricShader->GetPrimitiveMode();

    if (glPrimitiveMode == GL_LINES_ADJACENCY) {
        _genCommon << "#define HD_NUM_PRIMITIVE_VERTS 4\n";  // quad
    } else if (glPrimitiveMode == GL_PATCHES) {
        _genCommon << "#define HD_NUM_PATCH_VERTS "  // line=4, patch=16
                   << _geometricShader->GetPrimitiveIndexSize() << "\n";
        _genCommon << "#define HD_NUM_PRIMITIVE_VERTS 3\n";  // triangle
    } else {
        _genCommon << "#define HD_NUM_PRIMITIVE_VERTS 3\n";  // triangle
    }

    // include Glf ptex utility (if needed)
    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {
        HdBinding::Type bindingType = it->first.GetType();
        if (bindingType == HdBinding::TEXTURE_PTEX_TEXEL or
            bindingType == HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL) {
            _genCommon << _GetPtexTextureShaderSource();
            break;
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
   TF_FOR_ALL (it, _metaData.elementData) {
        _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
    }
    TF_FOR_ALL (it, _metaData.fvarData) {
        _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
    }
    TF_FOR_ALL (it, _metaData.vertexData) {
        _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
    }
    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {
        _genCommon << "#define HD_HAS_" << it->second.name << " 1\n";
    }

    // mixin shaders
    _genCommon << _geometricShader->GetSource(HdShaderTokens->commonShaderSource);
    TF_FOR_ALL(it, _shaders) {
        _genCommon << (*it)->GetSource(HdShaderTokens->commonShaderSource);
    }

    // prep interstage plumbing function
    _procVS  << "void ProcessPrimVars() {\n";
    _procTCS << "void ProcessPrimVars() {\n";
    _procTES << "void ProcessPrimVars(float u, float v, int i0, int i1, int i2, int i3) {\n";
    if (primType == PRIM_REFINED_QUAD or
        primType == PRIM_PATCH) {
        // patch interpolation
        _procGS  << "vec4 GetPatchCoord(int index);\n"
                 << "void ProcessPrimVars(int index) {\n"
                 << "   vec2 localST = GetPatchCoord(index).xy;\n";
    } else if (primType == PRIM_COARSE_QUAD) {
        // quad interpolation
        _procGS  << "void ProcessPrimVars(int index) {\n"
                 << "   vec2 localST = vec2[](vec2(0,0), vec2(1,0), vec2(1,1), vec2(0,1))[index];\n";
    } else { // PRIM_TRI
        // barycentric interpolation
        _procGS  << "void ProcessPrimVars(int index) {\n"
                 << "   vec2 localST = vec2[](vec2(1,0), vec2(0,1), vec2(0,0))[index];\n";
    }

    // generate drawing coord and accessors
    _GenerateDrawingCoord();

    // generate primvars
    _GenerateConstantPrimVar();
    _GenerateInstancePrimVar();
    _GenerateElementPrimVar(primType);
    _GenerateVertexPrimVar(primType);

    //generate shader parameters
    _GenerateShaderParameters();

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

    bool hasVS  = (not vertexShader.empty());
    bool hasTCS = (not tessControlShader.empty());
    bool hasTES = (not tessEvalShader.empty());
    bool hasGS  = (not geometryShader.empty());
    bool hasFS  = (not fragmentShader.empty());

    // other shaders (renderpass, lighting, surface) first
    TF_FOR_ALL(it, _shaders) {
        HdShaderSharedPtr const &shader = *it;
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

    // compile shaders
    // note: _vsSource, _fsSource etc are used for diagnostics (see header)
    if (hasVS) {
        _vsSource = _genCommon.str() + _genVS.str();
        glslProgram->CompileShader(GL_VERTEX_SHADER, _vsSource);
    }
    if (hasFS) {
        _fsSource = _genCommon.str() + _genFS.str();
        glslProgram->CompileShader(GL_FRAGMENT_SHADER, _fsSource);
    }
    if (hasTCS) {
        _tcsSource = _genCommon.str() + _genTCS.str();
        glslProgram->CompileShader(GL_TESS_CONTROL_SHADER, _tcsSource);
    }
    if (hasTES) {
        _tesSource = _genCommon.str() + _genTES.str();
        glslProgram->CompileShader(GL_TESS_EVALUATION_SHADER, _tesSource);
    }
    if (hasGS) {
        _gsSource = _genCommon.str() + _genGS.str();
        glslProgram->CompileShader(GL_GEOMETRY_SHADER, _gsSource);
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

    if (not TF_VERIFY(not name.IsEmpty())) return;
    if (not TF_VERIFY(not type.IsEmpty(),
                      "Unknown dataType for %s",
                      name.GetText())) return;

    if (arraySize > 0) {
        if (not TF_VERIFY(bindingType == HdBinding::UNIFORM_ARRAY or
                          bindingType == HdBinding::DRAW_INDEX_INSTANCE_ARRAY or
                          bindingType == HdBinding::UBO or
                          bindingType == HdBinding::SSBO or
                          bindingType == HdBinding::BINDLESS_UNIFORM)) {
            // XXX: SSBO and BINDLESS_UNIFORM don't need arraySize, but for the
            // workaround of UBO allocation we're passing arraySize = 2
            // for all bindingType.
            return;
        }
    }

    // layout qualifier (if exists)
    str << LayoutQualifier(binding);

    switch (bindingType) {
    case HdBinding::VERTEX_ATTR:
    case HdBinding::DRAW_INDEX:
    case HdBinding::DRAW_INDEX_INSTANCE:
        str << "in " << type.GetText() << " " << name.GetText() << ";\n";
        break;
    case HdBinding::DRAW_INDEX_INSTANCE_ARRAY:
        str << "in " << type.GetText() << " " << name.GetText()
            << "[" << arraySize << "];\n";
        break;
    case HdBinding::UNIFORM:
        str << "uniform " << type.GetText() << " " << name.GetText() << ";\n";
        break;
    case HdBinding::UNIFORM_ARRAY:
        str << "uniform " << type.GetText() << " " << name.GetText()
            << "[" << arraySize << "];\n";
        break;
    case HdBinding::UBO:
        // note: ubo_ prefix is used in HdResourceBinder::IntrospectBindings.
        str << "uniform ubo_" << name.GetText() <<  " {\n"
            << "  " << _GetPackedType(type).GetText()
            << " " << name.GetText();
        if (arraySize > 0) {
            str << "[" << arraySize << "];\n";
        } else {
            str << ";\n";
        }
        str << "};\n";
        break;
    case HdBinding::SSBO:
        str << "buffer buffer_" << binding.GetLocation() << " {\n"
            << "  " << _GetPackedType(type).GetText()
            << " " << name.GetText() << "[];\n"
            << "};\n";
        break;
    case HdBinding::TBO:
        str << "uniform " << _GetSamplerBufferType(type).GetText()
            << " " << name.GetText() << ";\n";
        break;
    case HdBinding::BINDLESS_UNIFORM:
        str << "uniform " << _GetPackedType(type).GetText()
            << " *" << name.GetText() << ";\n";
        break;
    case HdBinding::TEXTURE_2D:
    case HdBinding::BINDLESS_TEXTURE_2D:
        str << "uniform sampler2D " << name.GetText() << ";\n";
        break;
    case HdBinding::TEXTURE_PTEX_TEXEL:
        str << "uniform sampler2DArray " << name.GetText() << "_Data;\n";
        break;
    case HdBinding::TEXTURE_PTEX_LAYOUT:
        str << "uniform isamplerBuffer " << name.GetText() << "_Packing;\n";
        break;
    default:
        TF_CODING_ERROR("Unknown binding type %d, for %s\n",
                        binding.GetType(), name.GetText());
        break;
    }
}

static void _EmitDeclaration(
    std::stringstream &str,
    Hd_ResourceBinder::MetaData::BindingDeclaration const &bindingDeclaration,
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
                                const char *index = NULL)
{
    // index != NULL  if the struct is an array
    // arraySize > 1  if the struct entry is an array.
    if (index) {
        if (arraySize > 1) {
            str << type << " HdGet_" << name << "(int arrayIndex, int localIndex) {\n"
                // storing to a local variable to avoid the nvidia-driver
                // bug #1561110 (fixed in 346.59)
                << "  int index = " << index << ";\n"
                << "  return " << structName << "[index]." << name << "[arrayIndex];\n}\n";
        } else {
            str << type << " HdGet_" << name << "(int localIndex) {\n"
                << "  int index = " << index << ";\n"
                << "  return " << structName << "[index]." << name << ";\n}\n";
        }
    } else {
        if (arraySize > 1) {
            str << type << " HdGet_" << name << "(int arrayIndex, int localIndex) { return "
                << structName << "." << name << "[arrayIndex];}\n";
        } else {
            str << type << " HdGet_" << name << "(int localIndex) { return "
                << structName << "." << name << ";}\n";
        }
    }
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to localIndex=0
    if (arraySize > 1) {
        str << type << " HdGet_" << name << "(int arrayIndex)"
            << " { return HdGet_" << name << "(arrayIndex, 0); }\n";
    } else {
        str << type << " HdGet_" << name << "()"
            << " { return HdGet_" << name << "(0); }\n";
    }
}

static void _EmitAccessor(std::stringstream &str,
                          TfToken const &name,
                          TfToken const &type,
                          HdBinding const &binding,
                          const char *index)
{
    if (index) {
        str << type
            << " HdGet_" << name << "(int localIndex) {\n"
            << "  int index = " << index << ";\n";
        if (binding.GetType() == HdBinding::TBO) {

            std::string swizzle = "";
            if (type == _tokens->vec4 or type == _tokens->ivec4) {
                // nothing
            } else if (type == _tokens->vec3 or type == _tokens->ivec3) {
                swizzle = ".xyz";
            } else if (type == _tokens->vec2 or type == _tokens->ivec2) {
                swizzle = ".xy";
            } else if (type == _tokens->_float or type == _tokens->_int) {
                swizzle = ".x";
            }
            str << "  return texelFetch("
                << name << ", index)" << swizzle << ";\n}\n";
        } else {
            str << "  return " << _GetPackedTypeAccessor(type) << "("
                << name << "[index]);\n}\n";
        }
    } else {
        // non-indexed, only makes sense for uniform or vertex.
        if (binding.GetType() == HdBinding::UNIFORM or
            binding.GetType() == HdBinding::VERTEX_ATTR) {
            str << type
                << " HdGet_" << name << "(int localIndex) { return ";
            str << _GetPackedTypeAccessor(type) << "(" << name << ");}\n";
        }
    }
    // GLSL spec doesn't allow default parameter. use function overload instead.
    // default to locaIndex=0
    str << type << " HdGet_" << name << "()"
        << " { return HdGet_" << name << "(0); }\n";
    
}

void
Hd_CodeGen::_GenerateDrawingCoord()
{
    TF_VERIFY(_metaData.drawingCoord0Binding.binding.IsValid());
    TF_VERIFY(_metaData.drawingCoord1Binding.binding.IsValid());

    /*
       hd_drawingCoord is a struct of integer offsets to locate the primvars
       in buffer arrays at the current rendering location.

       struct hd_drawingCoord {
           int modelCoord;          // (reserved) model parameters
           int constantCoord;       // constant primvars (per object)
           int elementCoord;        // element primvars  (per face/curve)
           int primitiveCoord;      // primitive ids     (per tri/quad/line)
           int fvarCoord;           // fvar primvars     (per face-vertex)
           int shaderCoord;         // shader parameters (per shader/object)
           int instanceIndex[];     // (see below)
           int instanceCoords[];    // (see below)
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
               << "  int elementCoord;                            \n"
               << "  int primitiveCoord;                          \n"
               << "  int fvarCoord;                               \n"
               << "  int shaderCoord;                             \n"
               << "  int instanceIndex[HD_INSTANCE_INDEX_WIDTH];  \n"
               << "  int instanceCoords[HD_INSTANCE_INDEX_WIDTH]; \n"
               << "};\n";

    _genCommon << "hd_drawingCoord GetDrawingCoord();\n"; // forward declaration

    // vertex shader

    // [immediate]
    //   layout (location=x) uniform ivec4 drawingCoord0;
    //   layout (location=y) uniform ivec3 drawingCoord1;
    //   layout (location=z) uniform int   drawingCoordI[N];
    // [indirect]
    //   layout (location=x) in ivec4 drawingCoord0
    //   layout (location=y) in ivec3 drawingCoord1
    //   layout (location=z) in int   drawingCoordI[N]
    _EmitDeclaration(_genVS, _metaData.drawingCoord0Binding);
    _EmitDeclaration(_genVS, _metaData.drawingCoord1Binding);
    if (_metaData.drawingCoordIBinding.binding.IsValid()) {
        _EmitDeclaration(_genVS, _metaData.drawingCoordIBinding,
                         /*arraySize=*/std::max(1, _metaData.instancerNumLevels));
    }

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

        if (_geometricShader->IsCullingPass()) {
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
        if (_geometricShader->IsCullingPass()) {
            _genVS << "void SetCulledInstanceIndex(uint instance) "
                      "{ /*no-op*/ };\n";
        }
    }

    _genVS << "flat out hd_drawingCoord vsDrawingCoord;\n"
           // XXX: see the comment above why we need both vs and gs outputs.
           << "flat out hd_drawingCoord gsDrawingCoord;\n";

    _genVS << "hd_drawingCoord GetDrawingCoord() { hd_drawingCoord dc; \n"
           << "  dc.modelCoord     = drawingCoord0.x; \n"
           << "  dc.constantCoord  = drawingCoord0.y; \n"
           << "  dc.elementCoord   = drawingCoord0.z; \n"
           << "  dc.primitiveCoord = drawingCoord0.w; \n"
           << "  dc.fvarCoord      = drawingCoord1.x; \n"
           << "  dc.shaderCoord    = drawingCoord1.z; \n"
           << "  dc.instanceIndex  = GetInstanceIndex().indices;\n";

    if (_metaData.drawingCoordIBinding.binding.IsValid()) {
        _genVS << "  for (int i = 0; i < HD_INSTANCER_NUM_LEVELS; ++i) {\n"
               << "    dc.instanceCoords[i] = drawingCoordI[i] \n"
               << "      + GetInstanceIndex().indices[i+1]; \n"
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
            << "flat out hd_drawingCoord tcsDrawingCoord[HD_NUM_PATCH_VERTS];\n"
            << "hd_drawingCoord GetDrawingCoord() { \n"
            << "  hd_drawingCoord dc = vsDrawingCoord[gl_InvocationID];\n"
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
           << "  dc.primitiveCoord += gl_PrimitiveIDIn; \n"
           << "  return dc; \n"
           << "}\n";

    // fragment shader ( VSdc + gl_PrimitiveID )
    // note that gsDrawingCoord isn't offsetted by gl_PrimitiveIDIn
    _genFS << "flat in hd_drawingCoord gsDrawingCoord;\n"
           << "hd_drawingCoord GetDrawingCoord() { \n"
           << "  hd_drawingCoord dc = gsDrawingCoord; \n"
           << "  dc.primitiveCoord += gl_PrimitiveID; \n"
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
Hd_CodeGen::_GenerateConstantPrimVar()
{
    /*
      // --------- constant data delcaration ----------
      struct ConstantData0 {
          mat4 transform;
          mat4 transformInverse;
          mat4 instancerTransform[2];
          vec4 color;
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
      vec4 HdGet_color(int localIndex) {
          return constantData0[GetConstantCoord()].color;
      }

    */

    std::stringstream declarations;
    std::stringstream accessors;
    TF_FOR_ALL (it, _metaData.constantData) {
        // note: _constantData has been sorted by offset in Hd_ResourceBinder.
        // XXX: not robust enough, should consider padding and layouting rules
        // to match with the logic in HdInterleavedMemoryManager if we
        // want to use a layouting policy other than default padding.

        HdBinding binding = it->first;
        TfToken typeName(TfStringPrintf("ConstantData%d", binding.GetValue()));
        TfToken varName = it->second.blockName;

        declarations << "struct " << typeName << " {\n";

        TF_FOR_ALL (dbIt, it->second.entries) {
            if (not TF_VERIFY(not dbIt->dataType.IsEmpty(),
                              "Unknown dataType for %s",
                              dbIt->name.GetText())) {
                continue;
            }

            declarations << "  " << dbIt->dataType
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
Hd_CodeGen::_GenerateInstancePrimVar()
{
    /*
      // --------- instance data delcaration ----------
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
        accessors << it->second.dataType
                  << " HdGetInstance_" << it->first << "(int level, "
                  << it->second.dataType << " defaultValue) {\n";
        TF_FOR_ALL (levelIt, it->second.levels) {
            accessors << "  if (level == " << *levelIt << ") "
                      << "return HdGet_" << it->first << "_" << *levelIt << "();\n";
        }

        accessors << "  return defaultValue;\n"
                  << "}\n";
    }

    _genCommon << declarations.str()
               << accessors.str();
}

void
Hd_CodeGen::_GenerateElementPrimVar(int primType)
{
    // XXX: We'd like to return here, but can't because stub functions
    //      must be generated (see XXX comment below).
    //if (not (_metaData.primitiveParamBinding.binding.IsValid())) return;

    /*
      // --------- uniform (element) data delcaration ----------
      struct PrimitiveData { int elementID; }
      layout (std430, binding=?) buffer PrimitiveBuffer {
          PrimtiveData primitiveData[];
      };
      int GetElementID() {
          return primitiveData[GetPrimitiveCoord()].elementID
              + GetElementCoord();
      }

      struct ElementData0 {
          vec4 color;
      };
      layout (std430, binding=?) buffer buffer0 {
          ElementData0 elementData0[];
      };

      // --------- uniform data accessors ----------
      vec4 HdGet_color(int localIndex) {
          return elementData0[GetElementID()].color;
      }

    */
    std::stringstream declarations;
    std::stringstream accessors;


    // primitive param buffer can be one of followings:

    // 1. tris
    //     1 int  : coarse face index + edge flag
    //
    // 2. quads coarse
    //     2 ints : coarse face index + edge flag
    //              ptex index
    //
    // 3. quads uniformly refined
    //     3 ints : coarse face index + edge flag
    //              Far::PatchParam::field0 (includes ptex index)
    //              Far::PatchParam::field1
    //
    // 3. patch adaptively refined
    //     4 ints : coarse face index + edge flag
    //              Far::PatchParam::field0 (includes ptex index)
    //              Far::PatchParam::field1
    //              sharpness (float)

    //
    // note: decoding logic of primitiveParam has to match with
    // HdMeshTopology::DecodeFaceIndexFromPrimitiveParam()
    //
    // PatchParam is defined as ivec3 (see opensubdiv/far/patchParam.h)
    //  Field0     | Bits | Content
    //  -----------|:----:|---------------------------------------------------
    //  faceId     | 28   | the faceId of the patch (Hydra uses ptexIndex)
    //  transition | 4    | transition edge mask encoding
    //
    //  Field1     | Bits | Content
    //  -----------|:----:|---------------------------------------------------
    //  level      | 4    | the subdivision level of the patch
    //  nonquad    | 1    | whether the patch is the child of a non-quad face
    //  unused     | 3    | unused
    //  boundary   | 4    | boundary edge mask encoding
    //  v          | 10   | log2 value of u parameter at first patch corner
    //  u          | 10   | log2 value of v parameter at first patch corner
    //
    //  Field2     (float)  sharpness
    //
    // whereas adaptive patches have PatchParams computed by OpenSubdiv,
    // we need to construct PatchParams for coarse tris and quads.
    // Currently it's enough to fill just faceId for coarse quads for
    // ptex shading.

    if (_metaData.primitiveParamBinding.binding.IsValid()) {
        HdBinding binding = _metaData.primitiveParamBinding.binding;

        _EmitDeclaration(declarations, _metaData.primitiveParamBinding);
        _EmitAccessor(accessors, _metaData.primitiveParamBinding.name,
                      _metaData.primitiveParamBinding.dataType, binding,
                      "GetDrawingCoord().primitiveCoord");

        if (primType == PRIM_COARSE_QUAD) {
            // coarse quads (for ptex)
            // put ptexIndex into the first element of PatchParam.
            // (transition flags in MSB can be left as 0)
            accessors
                << "ivec3 GetPatchParam() {\n"
                << "  return ivec3(HdGet_primitiveParam().y, 0, 0);\n"
                << "}\n";
            accessors
                << "int GetEdgeFlag(int localIndex) {\n"
                << "  return localIndex; \n"
                << "}\n";
        } else if (primType == PRIM_REFINED_QUAD) {
            // refined quads
            accessors
                << "ivec3 GetPatchParam() {\n"
                << "  return ivec3(HdGet_primitiveParam().y, \n"
                << "               HdGet_primitiveParam().z, 0);\n"
                << "}\n";
            accessors
                << "int GetEdgeFlag(int localIndex) {\n"
                << "  return (HdGet_primitiveParam().x & 3);\n"
                << "}\n";
        } else if (primType == PRIM_PATCH) {
            // refined patches (tessellated triangles)
            accessors
                << "ivec3 GetPatchParam() {\n"
                << "  return ivec3(HdGet_primitiveParam().y, \n"
                << "               HdGet_primitiveParam().z, \n"
                << "               HdGet_primitiveParam().w);\n"
                << "}\n";
            accessors
                << "int GetEdgeFlag(int localIndex) {\n"
                << "  return localIndex;\n"
                << "}\n";
        } else {
            // coarse triangles, all other primitives
            //
            // note that triangulated meshes don't have ptexIndex.
            // Here we're passing primitiveID as ptexIndex PatchParam
            // since Hd_TriangulateFaceVaryingComputation unrolls facevaring
            // primvars for each triangles.
            accessors
                << "ivec3 GetPatchParam() {\n"
                << "  return ivec3(gl_PrimitiveID, 0, 0);\n"
                << "}\n";
            accessors
                << "int GetEdgeFlag(int localIndex) {\n"
                << "  return HdGet_primitiveParam() & 3;\n"
                << "}\n";
        }

        accessors
            << "int GetElementID() {\n"
            << "  return (hd_int_get(HdGet_primitiveParam()) >> 2)\n"
            << "  + GetDrawingCoord().elementCoord;\n"
            << "}\n";

        // note: fvar primvars are always quadrangulated or triangulated
        //       (= ptex-ified)
        if (primType == PRIM_TRI) {
            accessors
                << "int GetFVarIndex(int localIndex) {\n"
                << "  int fvarCoord = GetDrawingCoord().fvarCoord;\n"
                << "  int ptexIndex = GetPatchParam().x & 0xfffffff;\n"
                << "  return fvarCoord + ptexIndex * 3 + localIndex;\n"
                << "}\n";
        } else {
            accessors
                << "int GetFVarIndex(int localIndex) {\n"
                << "  int fvarCoord = GetDrawingCoord().fvarCoord;\n"
                << "  int ptexIndex = GetPatchParam().x & 0xfffffff;\n"
                << "  return fvarCoord + ptexIndex * 4 + localIndex;\n"
                << "}\n";
        }

    } else {
        // XXX: this is here only to keep the compiler happy, we don't expect
        // users to call them -- we really should restructure whatever is
        // necessary to avoid having to do this and thus guarantee that users
        // can never call bogus versions of these functions.
        accessors
            << "int GetElementID() {\n"
            << "  return 0;\n"
            << "}\n";
        accessors
            << "int GetEdgeFlag(int localIndex) {\n"
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
        << "int GetElementID();\n";

    TF_FOR_ALL (it, _metaData.elementData) {
        HdBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;

        _EmitDeclaration(declarations, name, dataType, binding);
        _EmitAccessor(accessors, name, dataType, binding, "GetElementID()");
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
Hd_CodeGen::_GenerateVertexPrimVar(int primType)
{
    /*
      // --------- vertex data delcaration (VS) ----------
      layout (location = 0) in vec3 normals;
      layout (location = 1) in vec3 points;

      out PrimVars {
          vec3 normals;
          vec3 points;
      } outPrimVars;

      void ProcessPrimVars() {
          outPrimVars.normals = normals;
          outPrimVars.points = points;
      }

      // --------- geometry stage plumbing -------
      in PrimVars {
          vec3 normals;
          vec3 points;
      } inPrimVars[];
      out PrimVars {
          vec3 normals;
          vec3 points;
      } outPrimVars;

      void ProcessPrimVars(int index) {
          outPrimVars = inPrimVars[index];
      }

      // --------- vertex data accessors (used in geometry/fragment shader) ---
      in PrimVars {
          vec3 normals;
          vec3 points;
      } inPrimVars;
      vec3 HdGet_normals(int localIndex=0) {
          return inPrimVars.normals;
      }
    */

    std::stringstream vertexInputs;
    std::stringstream interstageStruct;
    std::stringstream accessorsVS, accessorsTCS, accessorsTES,
        accessorsGS, accessorsFS;

    interstageStruct << "PrimVars {\n";

    // vertex varying
    TF_FOR_ALL (it, _metaData.vertexData) {
        HdBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;

        // future work:
        // with ARB_enhanced_layouts extention, it's possible
        // to use "component" qualifier to declare offsetted primvars
        // in interleaved buffer.
        _EmitDeclaration(vertexInputs, name, dataType, binding);

        interstageStruct << "  " << dataType << " " << name << ";\n";

        // primvar accessors
        _EmitAccessor(accessorsVS, name, dataType, binding);

        _EmitStructAccessor(accessorsTCS, _tokens->inPrimVars,
                            name, dataType, /*arraySize=*/1, "gl_InvocationID");
        _EmitStructAccessor(accessorsTES, _tokens->inPrimVars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsGS,  _tokens->inPrimVars,
                            name, dataType, /*arraySize=*/1, "localIndex");
        _EmitStructAccessor(accessorsFS,  _tokens->inPrimVars,
                            name, dataType, /*arraySize=*/1);

        // interstage plumbing
        _procVS << "  outPrimVars." << name
                << " = " << name << ";\n";
        _procTCS << "  outPrimVars[gl_InvocationID]." << name
                 << " = inPrimVars[gl_InvocationID]." << name << ";\n";
        // procTES linearly interpolate vertex/varying primvars here.
        // XXX: needs smooth interpolation for vertex primvars?
        _procTES << "  outPrimVars." << name
                 << " = mix(mix(inPrimVars[i3]." << name
                 << "         , inPrimVars[i2]." << name << ", u),"
                 << "       mix(inPrimVars[i1]." << name
                 << "         , inPrimVars[i0]." << name << ", u), v);\n";
        _procGS  << "  outPrimVars." << name
                 << " = inPrimVars[index]." << name << ";\n";
    }

    /*
      // --------- facevarying data declaration ----------------
      // we use separate structs to avoid std430 padding problem of vec3 array.
      struct FaceVaryingData0 {
          vec2 map1;
      };
      struct FaceVaryingData1 {
          float map2_u;
      };
      layout (std430, binding=?) buffer buffer0 {
          FaceVaryingData0 faceVaryingData0[];
      };
      layout (std430, binding=?) buffer buffer1 {
          FaceVaryingData1 faceVaryingData1[];
      };

      // --------- geometry stage plumbing -------
      void ProcessPrimVars(int index) {
          outPrimVars = inPrimVars[index];
      }

      // --------- facevarying data accessors ----------
      // in geometry shader
      vec2 HdGet_map1(int localIndex) {
          return faceVaryingData0[GetFaceVaryingIndex(localIndex)].map1;
      }
      // in fragment shader
      vec2 HdGet_map1() {
          return inPrimvars.map1;
      }

    */

    // face varying
    std::stringstream fvarDeclarations;

    TF_FOR_ALL (it, _metaData.fvarData) {
        HdBinding binding = it->first;
        TfToken const &name = it->second.name;
        TfToken const &dataType = it->second.dataType;

        _EmitDeclaration(fvarDeclarations, name, dataType, binding);

        interstageStruct << "  " << dataType << " " << name << ";\n";

        // primvar accessors (only in GS and FS)
        _EmitAccessor(accessorsGS, name, dataType, binding, "GetFVarIndex(localIndex)");
        _EmitStructAccessor(accessorsFS, _tokens->inPrimVars, name, dataType,
                            /*arraySize=*/1, NULL);

        // interstage plumbing
        _procVS << "  outPrimVars." << name
                << " = " << dataType << "(0);\n";
        _procTCS << "  outPrimVars[gl_InvocationID]." << name
                 << " = inPrimVars[gl_InvocationID]." << name << ";\n";
        // TODO: facevarying tessellation
        _procTES << "  outPrimVars." << name
                 << " = mix(mix(inPrimVars[i3]." << name
                 << "         , inPrimVars[i2]." << name << ", u),"
                 << "       mix(inPrimVars[i1]." << name
                 << "         , inPrimVars[i0]." << name << ", u), v);\n";

        if (primType == PRIM_COARSE_QUAD ||
            primType == PRIM_REFINED_QUAD ||
            primType == PRIM_PATCH) {
            // linear interpolation within a quad.
            _procGS << "   outPrimVars." << name
                    << "  = mix("
                    << "mix(" << "HdGet_" << name << "(0),"
                    <<           "HdGet_" << name << "(1), localST.x),"
                    << "mix(" << "HdGet_" << name << "(3),"
                    <<           "HdGet_" << name << "(2), localST.x), localST.y);\n";
        } else if (primType == PRIM_TRI) {
            // barycentric interpolation within a triangle.
            _procGS << "   outPrimVars." << name
                    << "  = HdGet_" << name << "(0) * localST.x "
                    << "  + HdGet_" << name << "(1) * localST.y "
                    << "  + HdGet_" << name << "(2) * (1-localST.x-localST.y);\n";
        }
    }

    interstageStruct << "}";

    _genVS << vertexInputs.str()
           << "out " << interstageStruct.str()
           << " outPrimVars;\n"
           << accessorsVS.str();

    _genTCS << "in " << interstageStruct.str()
            << " inPrimVars[gl_MaxPatchVertices];\n"
            << "out " << interstageStruct.str()
            << " outPrimVars[HD_NUM_PATCH_VERTS];\n"
            << accessorsTCS.str();

    _genTES << "in " << interstageStruct.str()
            << " inPrimVars[gl_MaxPatchVertices];\n"
            << "out " << interstageStruct.str()
            << " outPrimVars;\n"
            << accessorsTES.str();

    _genGS << fvarDeclarations.str()
           << "in " << interstageStruct.str()
           << " inPrimVars[HD_NUM_PRIMITIVE_VERTS];\n"
           << "out " << interstageStruct.str()
           << " outPrimVars;\n"
           << accessorsGS.str();

    _genFS << "in " << interstageStruct.str()
           << " inPrimVars;\n"
           << accessorsFS.str();

    // ---------
    _genFS << "vec4 GetPatchCoord(int index);\n";
    _genFS << "vec4 GetPatchCoord() { return GetPatchCoord(0); }\n";

    _genGS << "vec4 GetPatchCoord(int localIndex);\n";
}

void
Hd_CodeGen::_GenerateShaderParameters()
{
    /*
      ------------- Declarations -------------

      // shader parameter buffer
      struct ShaderData {
          <type>          <name>;
          vec4            diffuseColor;     // fallback uniform
          sampler2D       kdTexture;        // uv texture    (bindless texture)
          sampler2DArray  ptexTexels;       // ptex texels   (bindless texture)
          isamplerBuffer  ptexLayouts;      // ptex layouts  (bindless texture)
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
          return texture(sampler2D(shaderData[GetDrawingCoord().shaderCoord].<name>), <inPrimVars>).xxx;
      }

      * non-bindless 2D texture
      <type> HdGet_<name>(int localIndex=0) {
          return texture(samplers_2d[<offset> + drawIndex * <stride>], <inPrimVars>).xxx;
      }

      * bindless Ptex texture
      <type> HdGet_<name>(int localIndex=0) {
          return GlopPtexTextureLookup(<name>_Data, <name>_Packing, GetPatchCoord()).xxx;
      }

      * non-bindless Ptex texture
      <type> HdGet_<name>(int localIndex=0) {
          return GlopPtexTextureLookup(
              samplers_2darray[<offset_ptex_texels> + drawIndex * <stride>],
              isamplerBuffers[<offset_ptex_layouts> + drawIndex * <stride>],
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
              isamplerBuffers[<offset_ptex_layouts> + drawIndex * <stride>],
              patchCoord).xxx;
      }

    */

    std::stringstream declarations;
    std::stringstream accessors;

    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    TfToken typeName("ShaderData");
    TfToken varName("shaderData");

    // for shader parameters, we create declarations and accessors separetely.
    TF_FOR_ALL (it, _metaData.shaderData) {
        HdBinding binding = it->first;

        declarations << "struct " << typeName << " {\n";

        TF_FOR_ALL (dbIt, it->second.entries) {
            declarations << "  " << dbIt->dataType
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

    // accessors.
    TF_FOR_ALL (it, _metaData.shaderParameterBinding) {

        // adjust datatype
        std::string swizzle = "";
        if (it->second.dataType == _tokens->vec4) {
            // nothing
        } else if (it->second.dataType == _tokens->vec3) {
            swizzle = ".xyz";
        } else if (it->second.dataType == _tokens->vec2) {
            swizzle = ".xy";
        } else if (it->second.dataType == _tokens->_float) {
            swizzle = ".x";
        }

        HdBinding::Type bindingType = it->first.GetType();
        if (bindingType == HdBinding::FALLBACK) {
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name << "() {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return shaderData[shaderCoord]." << it->second.name << swizzle << ";\n"
                << "}\n";
        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_2D) {
            // a function returning sampler2D is allowed in 430 or later
            if (caps.glslVersion >= 430) {
                accessors
                    << "sampler2D\n"
                    << "HdGetSampler_" << it->second.name << "() {\n"
                    << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                    << "  return sampler2D(shaderData[shaderCoord]." << it->second.name << ");\n"
                    << "  }\n";
            }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name << "() {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return texture(sampler2D(shaderData[shaderCoord]." << it->second.name << "), ";

            if (not it->second.inPrimVars.empty()) {
                accessors 
                    << "\n"
                    << "#if defined(HD_HAS_" << it->second.inPrimVars[0] << ")\n"
                    << " HdGet_" << it->second.inPrimVars[0] << "().xy\n"
                    << "#else\n"
                    << "vec2(0.0, 0.0)\n"
                    << "#endif\n";
            } else {
            // allow to fetch uv texture without sampler coordinate for convenience.
                accessors
                    << " vec2(0.0, 0.0)";
            }
            accessors
                << ")" << swizzle << ";\n"
                << "}\n";
        } else if (bindingType == HdBinding::TEXTURE_2D) {
            declarations
                << LayoutQualifier(it->first)
                << "uniform sampler2D sampler2d_" << it->second.name << ";\n";
            // a function returning sampler2D is allowed in 430 or later
            if (caps.glslVersion >= 430) {
                accessors
                    << "sampler2D\n"
                    << "HdGetSampler_" << it->second.name << "() {\n"
                    << "  return sampler2d_" << it->second.name << ";"
                    << "}\n";
            }
            // vec4 HdGet_name(vec2 coord) { return texture(sampler2d_name, coord).xyz; }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "(vec2 coord) { return texture(sampler2d_"
                << it->second.name << ", coord)" << swizzle << ";}\n";
            // vec4 HdGet_name() { return HdGet_name(HdGet_st().xy); }
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name
                << "() { return HdGet_" << it->second.name << "(";
            if (not it->second.inPrimVars.empty()) {
                accessors
                    << "\n"
                    << "#if defined(HD_HAS_" << it->second.inPrimVars[0] << ")\n"
                    << "HdGet_" << it->second.inPrimVars[0] << "().xy\n"
                    << "#else\n"
                    << "vec2(0.0, 0.0)\n"
                    << "#endif\n";
            } else {
                accessors
                    << "vec2(0.0, 0.0)";
            }
            accessors << "); }\n";
        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_PTEX_TEXEL) {
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name << "(int localIndex) {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return " << it->second.dataType
                << "(GlopPtexTextureLookup("
                << "sampler2DArray(shaderData[shaderCoord]." << it->second.name <<"),"
                << "isamplerBuffer(shaderData[shaderCoord]." << it->second.name << "_layout), "
                << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                << "}\n"
                << it->second.dataType
                << " HdGet_" << it->second.name << "()"
                << "{ return HdGet_" << it->second.name << "(0); }\n"
                << it->second.dataType
                << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                << "  int shaderCoord = GetDrawingCoord().shaderCoord; \n"
                << "  return " << it->second.dataType
                << "(GlopPtexTextureLookup("
                << "sampler2DArray(shaderData[shaderCoord]." << it->second.name <<"),"
                << "isamplerBuffer(shaderData[shaderCoord]." << it->second.name << "_layout), "
                << "patchCoord)" << swizzle << ");\n"
                << "}\n";
        } else if (bindingType == HdBinding::TEXTURE_PTEX_TEXEL) {
            // +1 for layout is by convention.
            declarations
                << LayoutQualifier(it->first)
                << "uniform sampler2DArray sampler2darray_" << it->first.GetLocation() << ";\n"
                << LayoutQualifier(HdBinding(it->first.GetType(),
                                             it->first.GetLocation()+1,
                                             it->first.GetTextureUnit()))
                << "uniform isamplerBuffer isamplerbuffer_" << (it->first.GetLocation()+1) << ";\n";
            accessors
                << it->second.dataType
                << " HdGet_" << it->second.name << "(int localIndex) {\n"
                << "  return " << it->second.dataType
                << "(GlopPtexTextureLookup("
                << "sampler2darray_" << it->first.GetLocation() << ","
                << "isamplerbuffer_" << (it->first.GetLocation()+1) << ","
                << "GetPatchCoord(localIndex))" << swizzle << ");\n"
                << "}\n"
                << it->second.dataType
                << " HdGet_" << it->second.name << "()"
                << "{ return HdGet_" << it->second.name << "(0); }\n"
                << it->second.dataType
                << " HdGet_" << it->second.name << "(vec4 patchCoord) {\n"
                << "  return " << it->second.dataType
                << "(GlopPtexTextureLookup("
                << "sampler2darray_" << it->first.GetLocation() << ","
                << "isamplerbuffer_" << (it->first.GetLocation()+1) << ","
                << "patchCoord)" << swizzle << ");\n"
                << "}\n";
        } else if (bindingType == HdBinding::BINDLESS_TEXTURE_PTEX_LAYOUT) {
            //accessors << it->second.dataType << "(0)";
        } else if (bindingType == HdBinding::TEXTURE_PTEX_LAYOUT) {
            //accessors << it->second.dataType << "(0)";
        } else if (bindingType == HdBinding::PRIMVAR_REDIRECT) {
            // XXX: shader and primvar name collisions are a problem!
            // If this shader and it's connected primvar have the same name, we
            // are good to go, else we must alias the parameter to the primvar
            // accessor.
            if (it->second.name != it->second.inPrimVars[0]) {
                accessors
                    << it->second.dataType
                    << " HdGet_" << it->second.name << "() {\n"
                    << "#if defined(HD_HAS_" << it->second.inPrimVars[0] << ")\n"
                    << "  return HdGet_" << it->second.inPrimVars[0] << "();\n"
                    << "#else\n"
                    << "  return " << it->second.dataType << "(0);\n"
                    << "#endif\n"
                    << "\n}\n"
                    ;
            }
        }
    }
    
    _genFS << declarations.str()
           << accessors.str();

    _genGS << declarations.str()
           << accessors.str();
}
