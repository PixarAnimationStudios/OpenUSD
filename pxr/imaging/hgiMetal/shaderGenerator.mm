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

#include "pxr/imaging/hgiMetal/shaderGenerator.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgi/tokens.h"

#include <sstream>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (bufferBindings)
    (samplerBindings)
    (textureBindings)
);

template<typename SectionType, typename ...T>
SectionType *
HgiMetalShaderGenerator::CreateShaderSection(T && ...t)
{
    std::unique_ptr<SectionType> p =
        std::make_unique<SectionType>(std::forward<T>(t)...);
    SectionType * const result = p.get();
    GetShaderSections()->push_back(std::move(p));
    return result;
}

namespace {

//This is a conversion layer from descriptors into shader sections
//In purity we don't want the shader generator to know how to
//turn descriptors into sections, it is more interested in
//writing abstract sections
class ShaderStageData final
{
public:
    ShaderStageData(
        const HgiShaderFunctionDesc &descriptor,
        HgiMetalShaderGenerator *generator);

    HgiMetalShaderSectionPtrVector AccumulateParams(
        const HgiShaderFunctionParamDescVector &params,
        HgiMetalShaderGenerator *generator,
        HgiShaderStage stage,
        bool iterateAttrs);
    HgiMetalInterstageBlockShaderSectionPtrVector AccumulateParamBlocks(
        const HgiShaderFunctionParamBlockDescVector &params,
        HgiMetalShaderGenerator *generator,
        HgiShaderStage stage);
    HgiMetalShaderSectionPtrVector AccumulateBufferBindings(
        const HgiShaderFunctionBufferDescVector &buffers,
        HgiMetalShaderGenerator *generator);
    HgiMetalShaderSectionPtrVector AccumulateTextureBindings(
        const HgiShaderFunctionTextureDescVector &textures,
        HgiMetalShaderGenerator *generator);

    const HgiMetalShaderSectionPtrVector& GetConstantParams() const;
    const HgiMetalShaderSectionPtrVector& GetInputs() const;
    const HgiMetalShaderSectionPtrVector& GetOutputs() const;
    const HgiMetalShaderSectionPtrVector& GetBufferBindings() const;
    const HgiMetalShaderSectionPtrVector& GetSamplerBindings() const;
    const HgiMetalShaderSectionPtrVector& GetTextureBindings() const;

    const std::string inputsGenericWrapper;
    const std::string inputsGenericParameters;

private:
    ShaderStageData() = delete;
    ShaderStageData & operator=(const ShaderStageData&) = delete;
    ShaderStageData(const ShaderStageData&) = delete;

    const HgiMetalShaderSectionPtrVector _constantParams;
    const HgiMetalInterstageBlockShaderSectionPtrVector _inputBlocks;
    const HgiMetalInterstageBlockShaderSectionPtrVector _outputBlocks;
    const HgiMetalShaderSectionPtrVector _inputs;
    const HgiMetalShaderSectionPtrVector _outputs;
    const HgiMetalShaderSectionPtrVector _bufferBindings;
    HgiMetalShaderSectionPtrVector _samplerBindings;
    HgiMetalShaderSectionPtrVector _textureBindings;
};

template<typename T>
T* _BuildStructInstance(
    const std::string &typeName,
    const std::string &instanceName,
    const std::string &attribute,
    const std::string &addressSpace,
    const bool isPointer,
    const HgiMetalShaderSectionPtrVector &members,
    HgiMetalShaderGenerator *generator,
    const std::string templateWrapper = std::string())
{
    //If it doesn't have any members, don't declare an empty struct instance
    if(typeName.empty() || members.empty()) {
        return nullptr;
    }

    HgiMetalStructTypeDeclarationShaderSection * const section =
        generator->CreateShaderSection<
            HgiMetalStructTypeDeclarationShaderSection>(
                typeName,
                members,
                templateWrapper);

    const HgiShaderSectionAttributeVector attributes = {
        HgiShaderSectionAttribute{attribute, ""}};

    return generator->CreateShaderSection<T>(
        instanceName,
        attributes,
        addressSpace,
        isPointer,
        section);
}

bool
_GetBuiltinKeyword(HgiShaderFunctionParamDesc const &param,
                   std::string *keyword = nullptr)
{
    //possible metal attributes on shader inputs.
    // Map from descriptor to Metal
    const static std::unordered_map<std::string, std::string> roleIndexM {
       {HgiShaderKeywordTokens->hdVertexID, "vertex_id"},
       {HgiShaderKeywordTokens->hdInstanceID, "instance_id"},
       {HgiShaderKeywordTokens->hdBaseVertex, "base_vertex"},
       {HgiShaderKeywordTokens->hdBaseInstance, "base_instance"},
       {HgiShaderKeywordTokens->hdGlobalInvocationID, "thread_position_in_grid"},
       {HgiShaderKeywordTokens->hdPatchID, "patch_id"},
       {HgiShaderKeywordTokens->hdPositionInPatch, "position_in_patch"},
       {HgiShaderKeywordTokens->hdPrimitiveID, "primitive_id"},
       {HgiShaderKeywordTokens->hdFrontFacing, "front_facing"},
       {HgiShaderKeywordTokens->hdPosition, "position"},
       {HgiShaderKeywordTokens->hdBaryCoordNoPersp, "barycentric_coord"},
       {HgiShaderKeywordTokens->hdFragCoord, "position"}
    };

    //check if has a role
    if(!param.role.empty()) {
        auto it = roleIndexM.find(param.role);
        if (it != roleIndexM.end()) {
            if (keyword) {
                *keyword = it->second;
            }
            return true;
        }
    }
    
    return false;
}

} // anonymous namespace

/// \class HgiMetalShaderStageEntryPoint
///
/// Generates a metal stage function. Base class for vertex/fragment/compute
///
class HgiMetalShaderStageEntryPoint final
{
public:    
    HgiMetalShaderStageEntryPoint(
          const ShaderStageData &stageData,
          HgiMetalShaderGenerator *generator,
          const std::string &outputShortHandPrefix,
          const std::string &scopePostfix,
          const std::string &entryPointStageName,
          const std::string &outputTypeName,
          const std::string &entryPointFunctionName,
          const std::string &entryPointAttributes);
    
    HgiMetalShaderStageEntryPoint(
          const ShaderStageData &stageData,
          HgiMetalShaderGenerator *generator,
          const std::string &outputShortHandPrefix,
          const std::string &scopePostfix,
          const std::string &entryPointStageName,
          const std::string &inputInstanceName,
          const std::string &entryPointAttributes);

    const std::string& GetOutputShortHandPrefix() const;
    const std::string& GetScopePostfix() const;
    const std::string& GetEntryPointStageName() const;
    const std::string& GetEntryPointAttributes() const;
    const std::string& GetEntryPointFunctionName() const;
    const std::string& GetOutputTypeName() const;
    const std::string& GetInputsInstanceName() const;
    
    std::string GetOutputInstanceName() const;
    const std::string& GetScopeInstanceName() const;
    std::string GetConstantBufferTypeName() const;
    std::string GetConstantBufferInstanceName() const;
    std::string GetScopeTypeName() const;
    std::string GetInputsTypeName() const;
    std::string GetBindingsBufferTypeName() const;
    std::string GetBindingsSamplerTypeName() const;
    std::string GetBindingsTextureTypeName() const;
    HgiMetalParameterInputShaderSection* GetParameters();
    HgiMetalParameterInputShaderSection* GetInputs();
    HgiMetalStageOutputShaderSection* GetOutputs();
    HgiMetalArgumentBufferInputShaderSection* GetBufferBindings();
    HgiMetalArgumentBufferInputShaderSection* GetTextureBindings();

private:
    void _Init(
        const HgiMetalShaderSectionPtrVector &stageConstantBuffers,
        const HgiMetalShaderSectionPtrVector &stageInputs,
        const HgiMetalShaderSectionPtrVector &stageOutputs,
        const HgiMetalShaderSectionPtrVector &stageBufferBindings,
        const HgiMetalShaderSectionPtrVector &stageSamplerBindings,
        const HgiMetalShaderSectionPtrVector &stageTextureBindings,
        HgiMetalShaderGenerator *generator);

    HgiMetalShaderStageEntryPoint & operator=(
        const HgiMetalShaderStageEntryPoint&) = delete;
    HgiMetalShaderStageEntryPoint(
        const HgiMetalShaderStageEntryPoint&) = delete;

    //Owned by and stored in shadersections
    HgiMetalParameterInputShaderSection* _parameters;
    HgiMetalParameterInputShaderSection* _inputs;
    HgiMetalStageOutputShaderSection* _outputs;
    HgiMetalArgumentBufferInputShaderSection* _bufferBindings;
    HgiMetalArgumentBufferInputShaderSection* _samplerBindings;
    HgiMetalArgumentBufferInputShaderSection* _textureBindings;
    const std::string _inputsGenericWrapper;
    const std::string _outputShortHandPrefix;
    const std::string _scopePostfix;
    const std::string _entryPointStageName;
    const std::string _outputTypeName;
    const std::string _entryPointFunctionName;
    const std::string _entryPointAttributes;
    const std::string _inputInstanceName;
};

namespace {

//This is used by the macro blob, basically this is dumped on top
//of the generated shader

const char *
_GetDeclarationDefinitions()
{
    return
        "#define REF(space,type) space type &\n"
        "#define FORWARD_DECL(...)\n"
        "#define ATOMIC_LOAD(a)"
        " atomic_load_explicit(&a, memory_order_relaxed)\n"
        "#define ATOMIC_STORE(a, v)"
        " atomic_store_explicit(&a, v, memory_order_relaxed)\n"
        "#define ATOMIC_ADD(a, v)"
        " atomic_fetch_add_explicit(&a, v, memory_order_relaxed)\n"
        "#define ATOMIC_EXCHANGE(a, desired)"
        " atomic_exchange_explicit(&a, desired, memory_order_relaxed)\n"
        "#define ATOMIC_COMP_SWAP(a, expected, desired) \\"
        "  atomic_compare_exchange_strong_explicit(&a, &expected, desired, "
        "memory_order_relaxed, memory_order_relaxed)\n"
        "\n";
}

static const char *
_GetPackedTypeDefinitions()
{
    return
    "struct hgi_ivec3 { int    x, y, z;\n"
    "  hgi_ivec3(int _x, int _y, int _z): x(_x), y(_y), z(_z) {}\n"
    "};\n"
    "struct hgi_vec3  { float  x, y, z;\n"
    "  hgi_vec3(float _x, float _y, float _z): x(_x), y(_y), z(_z) {}\n"
    "};\n"
    "struct hgi_dvec3 { double x, y, z;\n"
    "  hgi_dvec3(double _x, double _y, double _z): x(_x), y(_y), z(_z) {}\n"
    "};\n"
    "struct hgi_mat3 { float m00, m01, m02,\n"
    "                        m10, m11, m12,\n"
    "                        m20, m21, m22;\n"
    "  hgi_mat3(float _00, float _01, float _02, \\\n"
    "           float _10, float _11, float _12, \\\n"
    "           float _20, float _21, float _22) \\\n"
    "             : m00(_00), m01(_01), m02(_02) \\\n"
    "             , m10(_10), m11(_11), m12(_12) \\\n"
    "             , m20(_20), m21(_21), m22(_22) {}\n"
    "};\n"
    "struct hgi_dmat3 { double m00, m01, m02,\n"
    "                          m10, m11, m12,\n"
    "                          m20, m21, m22;\n"
    "  hgi_dmat3(double _00, double _01, double _02, \\\n"
    "            double _10, double _11, double _12, \\\n"
    "            double _20, double _21, double _22) \\\n"
    "              : m00(_00), m01(_01), m02(_02) \\\n"
    "              , m10(_10), m11(_11), m12(_12) \\\n"
    "              , m20(_20), m21(_21), m22(_22) {}\n"
    "};\n"
    "\n"
    "mat4 inverse_fast(float4x4 a) { return transpose(a); }\n"
    "mat4 inverse(float4x4 a) {\n"
    "    float b00 = a[0][0] * a[1][1] - a[0][1] * a[1][0];\n"
    "    float b01 = a[0][0] * a[1][2] - a[0][2] * a[1][0];\n"
    "    float b02 = a[0][0] * a[1][3] - a[0][3] * a[1][0];\n"
    "    float b03 = a[0][1] * a[1][2] - a[0][2] * a[1][1];\n"
    "    float b04 = a[0][1] * a[1][3] - a[0][3] * a[1][1];\n"
    "    float b05 = a[0][2] * a[1][3] - a[0][3] * a[1][2];\n"
    "    float b06 = a[2][0] * a[3][1] - a[2][1] * a[3][0];\n"
    "    float b07 = a[2][0] * a[3][2] - a[2][2] * a[3][0];\n"
    "    float b08 = a[2][0] * a[3][3] - a[2][3] * a[3][0];\n"
    "    float b09 = a[2][1] * a[3][2] - a[2][2] * a[3][1];\n"
    "    float b10 = a[2][1] * a[3][3] - a[2][3] * a[3][1];\n"
    "    float b11 = a[2][2] * a[3][3] - a[2][3] * a[3][2];\n"

    "    float invdet = 1.0 / (b00 * b11 - b01 * b10 + b02 * b09 +\n"
    "                          b03 * b08 - b04 * b07 + b05 * b06);\n"

    "    return mat4(a[1][1] * b11 - a[1][2] * b10 + a[1][3] * b09,\n"
    "                a[0][2] * b10 - a[0][1] * b11 - a[0][3] * b09,\n"
    "                a[3][1] * b05 - a[3][2] * b04 + a[3][3] * b03,\n"
    "                a[2][2] * b04 - a[2][1] * b05 - a[2][3] * b03,\n"
    "                a[1][2] * b08 - a[1][0] * b11 - a[1][3] * b07,\n"
    "                a[0][0] * b11 - a[0][2] * b08 + a[0][3] * b07,\n"
    "                a[3][2] * b02 - a[3][0] * b05 - a[3][3] * b01,\n"
    "                a[2][0] * b05 - a[2][2] * b02 + a[2][3] * b01,\n"
    "                a[1][0] * b10 - a[1][1] * b08 + a[1][3] * b06,\n"
    "                a[0][1] * b08 - a[0][0] * b10 - a[0][3] * b06,\n"
    "                a[3][0] * b04 - a[3][1] * b02 + a[3][3] * b00,\n"
    "                a[2][1] * b02 - a[2][0] * b04 - a[2][3] * b00,\n"
    "                a[1][1] * b07 - a[1][0] * b09 - a[1][2] * b06,\n"
    "                a[0][0] * b09 - a[0][1] * b07 + a[0][2] * b06,\n"
    "                a[3][1] * b01 - a[3][0] * b03 - a[3][2] * b00,\n"
    "                a[2][0] * b03 - a[2][1] * b01 + a[2][2] * b00) * invdet;\n"
    "}\n\n";
}

std::string
_ComputeHeader(id<MTLDevice> device, HgiShaderStage stage)
{
    std::stringstream header;

    // Metal feature set defines
    // Define all macOS 10.13 feature set enums onwards
    if (@available(macos 10.13, ios 100.100, *)) {
        header  << "#define ARCH_OS_MACOS\n";
        if ([device supportsFeatureSet:MTLFeatureSet(10003)])
            header << "#define METAL_FEATURESET_MACOS_GPUFAMILY1_v3\n";
    }
    if (@available(macos 10.14, ios 100.100, *)) {
        if ([device supportsFeatureSet:MTLFeatureSet(10004)])
            header << "#define METAL_FEATURESET_MACOS_GPUFAMILY1_v4\n";
    }
    if (@available(macos 10.14, ios 100.100, *)) {
        if ([device supportsFeatureSet:MTLFeatureSet(10005)])
            header << "#define METAL_FEATURESET_MACOS_GPUFAMILY2_v1\n";
    }

    if (@available(macos 100.100, ios 12.0, *)) {
        header  << "#define ARCH_OS_IOS\n";
        // Define all iOS 12 feature set enums onwards
        if ([device supportsFeatureSet:MTLFeatureSet(12)])
            header << "#define METAL_FEATURESET_IOS_GPUFAMILY1_v5\n";
    }
    if (@available(macos 100.100, ios 12.0, *)) {
        if ([device supportsFeatureSet:MTLFeatureSet(12)])
            header << "#define METAL_FEATURESET_IOS_GPUFAMILY2_v5\n";
    }
    if (@available(macos 100.100, ios 12.0, *)) {
        if ([device supportsFeatureSet:MTLFeatureSet(13)])
            header << "#define METAL_FEATURESET_IOS_GPUFAMILY3_v4\n";
    }
    if (@available(macos 100.100, ios 12.0, *)) {
        if ([device supportsFeatureSet:MTLFeatureSet(14)])
            header << "#define METAL_FEATURESET_IOS_GPUFAMILY4_v2\n";
    }

    header  << "#include <metal_stdlib>\n"
            << "#include <simd/simd.h>\n"
            << "#include <metal_pack>\n"
            << "#pragma clang diagnostic ignored \"-Wunused-variable\"\n"
            << "#pragma clang diagnostic ignored \"-Wsign-compare\"\n"
            << "using namespace metal;\n";

    // Basic types
    header  << "#define double float\n"
            << "#define vec2 float2\n"
            << "#define vec3 float3\n"
            << "#define vec4 float4\n"
            << "#define mat2 float2x2\n"
            << "#define mat3 float3x3\n"
            << "#define mat4 float4x4\n"
            << "#define ivec2 int2\n"
            << "#define ivec3 int3\n"
            << "#define ivec4 int4\n"
            << "#define uvec2 uint2\n"
            << "#define uvec3 uint3\n"
            << "#define uvec4 uint4\n"
            << "#define bvec2 bool2\n"
            << "#define bvec3 bool3\n"
            << "#define bvec4 bool4\n"
            << "#define dvec2 float2\n"
            << "#define dvec3 float3\n"
            << "#define dvec4 float4\n"
            << "#define dmat2 float2x2\n"
            << "#define dmat3 float3x3\n"
            << "#define dmat4 float4x4\n"
            << "#define usampler1DArray texture1d_array<uint16_t>\n"
            << "#define sampler2DArray texture2d_array<float>\n"
            << "#define sampler2DShadow depth2d<float>\n";

    // XXX: this macro is still used in GlobalUniform.
    header  << "#define MAT4 mat4\n";

    // macros to help with declarations
    header  << _GetDeclarationDefinitions();

    // a trick to tightly pack vec3 into SSBO/UBO.
    header  << _GetPackedTypeDefinitions();

    header << "#define in /*in*/\n"
              "#define discard discard_fragment(); discarded_fragment = true;\n"
              "#define radians(d) (d * 0.01745329252)\n"
              "#define noperspective /*center_no_perspective MTL_FIXME*/\n"
              "#define dFdx    dfdx\n"
              "#define dFdy    dfdy\n"

              "#define lessThan(a, b) ((a) < (b))\n"
              "#define lessThanEqual(a, b) ((a) <= (b))\n"
              "#define greaterThan(a, b) ((a) > (b))\n"
              "#define greaterThanEqual(a, b) ((a) >= (b))\n"
              "#define equal(a, b) ((a) == (b))\n"
              "#define notEqual(a, b) ((a) != (b))\n"
    
              "union HgiPackedf16 { uint i; half2 h; };\n"
              "vec2 unpackHalf2x16(uint val)\n"
              "{\n"
              "    HgiPackedf16 v;\n"
              "    v.i = val;\n"
              "    return vec2(v.h.x, v.h.y);\n"
              "}\n"
              "uint packHalf2x16(vec2 val)\n"
              "{\n"
              "    HgiPackedf16 v;\n"
              "    v.h = half2(val.x, val.y);\n"
              "    return v.i;\n"
              "}\n"

              "template <typename T>\n"
              "T mod(T y, T x) { return fmod(y, x); }\n\n"
              "template <typename T>\n"
              "T atan(T y, T x) { return atan2(y, x); }\n\n"
              "template <typename T>\n"
              "T bitfieldReverse(T x) { return reverse_bits(x); }\n\n"
              "template <typename T>\n"
              "T bitfieldExtract(T value, int offset, int bits) {\n"
              "  return extract_bits(value, offset, bits); }\n\n"
    
              "template <typename T>\n"
              "int imageSize1d(T texture) {\n"
              "    return int(texture.get_width());\n"
              "}\n"
              "template <typename T>\n"
              "ivec2 imageSize2d(T texture) {\n"
              "    return ivec2(texture.get_width(), texture.get_height());\n"
              "}\n"
              "template <typename T>\n"
              "ivec3 imageSize3d(T texture) {\n"
              "    return ivec3(texture.get_width(),\n"
              "        texture.get_height(), texture.get_depth());\n"
              "}\n"
    
              "template <typename T>\n"
              "ivec2 textureSize(T texture, uint lod = 0) {\n"
              "    return ivec2(texture.get_width(lod), texture.get_height(lod));\n"
              "}\n"
              "ivec2 textureSize(texture1d_array<uint16_t> texture, uint lod = 0) {\n"
              "    return ivec2(texture.get_width(),\n"
              "        texture.get_array_size());\n"
              "}\n"
              "ivec3 textureSize(texture2d_array<float> texture, uint lod = 0) {\n"
              "    return ivec3(texture.get_width(lod),\n"
              "        texture.get_height(lod), texture.get_array_size());\n"
              "}\n"
    
              "template <typename T>\n"
              "int textureSize1d(T texture, uint lod = 0) {\n"
              "    return int(texture.get_width());\n"
              "}\n"
              "template <typename T>\n"
              "ivec2 textureSize2d(T texture, uint lod = 0) {\n"
              "    return ivec2(texture.get_width(lod), texture.get_height(lod));\n"
              "}\n"
              "template <typename T>\n"
              "ivec3 textureSize3d(T texture, uint lod = 0) {\n"
              "    return ivec3(texture.get_width(lod),\n"
              "        texture.get_height(lod), texture.get_depth(lod));\n"
              "}\n\n"
    
              "template<typename T, typename Tc>\n"
              "float4 texelFetch(T texture, Tc coords, uint lod = 0) {\n"
              "    return texture.read(uint2(coords), lod);\n"
              "}\n"
              "template<typename Tc>\n"
              "uint4 texelFetch(texture1d_array<uint16_t> texture, Tc coords, uint lod = 0) {\n"
              "    return uint4(texture.read((uint)coords.x, (uint)coords.y, 0));\n"
              "}\n"
              "template<typename Tc>\n"
              "vec4 texelFetch(texture2d_array<float> texture, Tc coords, uint lod = 0) {\n"
              "    return texture.read(uint2(coords.xy), (uint)coords.z, 0);\n"
              "}\n"
    
              "#define textureQueryLevels(texture) texture.get_num_mip_levels()\n"

              "template <typename T, typename Tv>\n"
              "void imageStore(T texture, short2 coords, Tv color) {\n"
              "    return texture.write(color, ushort2(coords.x, coords.y));\n"
              "}\n"
              "template <typename T, typename Tv>\n"
              "void imageStore(T texture, int2 coords, Tv color) {\n"
              "    return texture.write(color, uint2(coords.x, coords.y));\n"
              "}\n\n"

              "constexpr sampler texelSampler(address::clamp_to_edge,\n"
              "                               filter::linear);\n"
    
              "template<typename T, typename Tc>\n"
              "float4 texture(T texture, Tc coords) {\n"
              "    return texture.sample(texelSampler, coords);\n"
              "}\n"
              "template<typename Tc>\n"
              "vec4 texture(texture2d_array<float> texture, Tc coords) {\n"
              "    return texture.sample(texelSampler, coords.xy, coords.z);\n"
              "}\n"

    ;

    if (stage & HgiShaderStageVertex) {
        header << "int HgiGetBaseVertex() {\n"
                    "  return 0;\n"
                    "}\n";
    }

    return header.str();
}

std::string const&
_GetHeader(id<MTLDevice> device, HgiShaderStage stage)
{
    // This assumes that there is only ever one MTLDevice.
    static std::string header = _ComputeHeader(device, stage);
    return header;
}

HgiMetalShaderSectionPtrVector
_AccumulateParamsAndBlockParams(
    const HgiMetalInterstageBlockShaderSectionPtrVector &blocks,
    const HgiMetalShaderSectionPtrVector &params)
{
    HgiMetalShaderSectionPtrVector result = params;
    for (const HgiMetalInterstageBlockShaderSection *block : blocks) {
        const HgiMetalShaderSectionPtrVector &members =
            block->GetStructTypeDeclaration()->GetMembers();
        result.insert(result.end(), members.begin(), members.end());
    }
    return result;
}

bool
_IsTessFunction(const HgiShaderFunctionDesc &descriptor)
{
    return descriptor.shaderStage == HgiShaderStagePostTessellationControl ||
           descriptor.shaderStage == HgiShaderStagePostTessellationVertex;
}

ShaderStageData::ShaderStageData(
    const HgiShaderFunctionDesc &descriptor,
    HgiMetalShaderGenerator *generator)
    : inputsGenericWrapper(
        _IsTessFunction(descriptor) ? "patch_control_point" : "")
    , _constantParams(
        AccumulateParams(
            descriptor.constantParams,
            generator,
            descriptor.shaderStage,
            false))
    , _inputBlocks(
        AccumulateParamBlocks(
            descriptor.stageInputBlocks,
            generator,
            descriptor.shaderStage))
    , _outputBlocks(
        AccumulateParamBlocks(
            descriptor.stageOutputBlocks,
            generator,
            descriptor.shaderStage))
    , _inputs(
        _AccumulateParamsAndBlockParams(
            _inputBlocks,
            AccumulateParams(
                descriptor.stageInputs,
                generator,
                descriptor.shaderStage,
                descriptor.shaderStage == HgiShaderStageVertex
                || descriptor.shaderStage ==
                  HgiShaderStagePostTessellationControl
                || descriptor.shaderStage ==
                  HgiShaderStagePostTessellationVertex)))
    , _outputs(
        _AccumulateParamsAndBlockParams(
            _outputBlocks,
            AccumulateParams(
                descriptor.stageOutputs,
                generator,
                descriptor.shaderStage,
                false)))
    , _bufferBindings(
        AccumulateBufferBindings(
            descriptor.buffers,
            generator))
{
    // Also populates _samplerBindings
    _textureBindings = AccumulateTextureBindings(
        descriptor.textures,
        generator);
}

//Convert ShaderFunctionParamDescs into shader sections
HgiMetalShaderSectionPtrVector 
ShaderStageData::AccumulateParams(
    const HgiShaderFunctionParamDescVector &params,
    HgiMetalShaderGenerator *generator,
    HgiShaderStage stage,
    bool iterateAttrs)
{
    HgiMetalShaderSectionPtrVector stageShaderSections;
    //only some roles have an index
    if(!iterateAttrs) {
        //possible metal attributes on shader inputs.
        // Map from descriptor to metal
        std::unordered_map<std::string, uint32_t> roleIndexM {
                {"color", 0}
        };

        for(const HgiShaderFunctionParamDesc &p : params) {
            if (_GetBuiltinKeyword(p)) continue;
            //For metal, the role is the actual attribute so far
            std::string indexAsStr;
            //check if has a role
            if(!p.role.empty()) {
                auto it = roleIndexM.find(p.role);
                if (it != roleIndexM.end()) {
                    indexAsStr = std::to_string(it->second);
                    //Increment index, so that the next color
                    //or texture or vertex has a higher index
                    (it)->second += 1;
                }
            }

            HgiShaderSectionAttributeVector attributes = {};
            if (!p.role.empty()) {
                attributes.push_back(HgiShaderSectionAttribute{p.role, indexAsStr});
            }
            else if (p.interstageSlot != -1) {
                std::string role = "user(slot" + std::to_string(p.interstageSlot) + ")";
                attributes.push_back(HgiShaderSectionAttribute{role, indexAsStr});
            }

            switch (p.interpolation) {
            case HgiInterpolationDefault:
                break;
            case HgiInterpolationFlat:
                attributes.push_back(
                    HgiShaderSectionAttribute{"flat", ""});
                break;
            case HgiInterpolationNoPerspective:
                attributes.push_back(
                    HgiShaderSectionAttribute{"center_no_perspective", ""});
                break;
            }

            HgiMetalMemberShaderSection * const section =
                generator->CreateShaderSection<
                    HgiMetalMemberShaderSection>(
                        p.nameInShader,
                        p.type,
                        attributes,
                        p.arraySize);
            stageShaderSections.push_back(section);
        }
    } else {
        int nextLocation = 0;
        for (size_t i = 0; i < params.size(); i++) {
            const HgiShaderFunctionParamDesc &p = params[i];
            if (_GetBuiltinKeyword(p)) continue;

            const int location =
                (p.location != -1) ? p.location : nextLocation;

            nextLocation = location + 1;

            const HgiShaderSectionAttributeVector attributes = {
                HgiShaderSectionAttribute{"attribute", std::to_string(location)}
            };

            HgiMetalMemberShaderSection * const section =
                generator->CreateShaderSection<
                    HgiMetalMemberShaderSection>(
                        p.nameInShader,
                        p.type,
                        attributes,
                        p.arraySize);
            stageShaderSections.push_back(section);
        }
    }
    return stageShaderSections;
}

HgiMetalInterstageBlockShaderSectionPtrVector 
ShaderStageData::AccumulateParamBlocks(
    const HgiShaderFunctionParamBlockDescVector &params,
    HgiMetalShaderGenerator *generator,
    HgiShaderStage stage)
{
    HgiMetalInterstageBlockShaderSectionPtrVector stageShaderSections;
    for(const HgiShaderFunctionParamBlockDesc &p : params) {

        HgiMetalShaderSectionPtrVector blockMembers;
        for (size_t i = 0; i < p.members.size(); ++i) {
            const HgiShaderFunctionParamBlockDesc::Member &m = p.members[i];
            const size_t slotIndex = p.interstageSlot + i;

            const std::string role =
                "user(slot" + std::to_string(slotIndex) + ")";
            const HgiShaderSectionAttributeVector attributes = { {role, ""} };

            HgiMetalMemberShaderSection * const memberSection =
                generator->CreateShaderSection<
                        HgiMetalMemberShaderSection>(
                            m.name,
                            m.type,
                            attributes,
                            std::string(),
                            p.instanceName);
            blockMembers.push_back(memberSection);
        }

        HgiMetalStructTypeDeclarationShaderSection * const blockStruct =
            generator->CreateShaderSection<
                HgiMetalStructTypeDeclarationShaderSection>(
                    p.blockName + "_" + p.instanceName,
                    blockMembers);

        HgiMetalInterstageBlockShaderSection * const blockSection =
            generator->CreateShaderSection<
                HgiMetalInterstageBlockShaderSection>(
                    p.blockName,
                    p.instanceName,
                    blockStruct);
        stageShaderSections.push_back(blockSection);
    }
    return stageShaderSections;
}

HgiMetalShaderSectionPtrVector
ShaderStageData::AccumulateBufferBindings(
    const HgiShaderFunctionBufferDescVector &buffers,
    HgiMetalShaderGenerator *generator)
{
    HgiMetalShaderSectionPtrVector stageShaderSections;
    uint32_t maxBindIndex = 0;

    std::vector<const HgiShaderFunctionBufferDesc*> slots(32, nullptr);
    for (size_t i = 0; i < buffers.size(); i++) {
        uint32_t bindIndex = buffers[i].bindIndex;
        maxBindIndex = std::max(maxBindIndex, bindIndex);
        if (maxBindIndex >= slots.size()) {
            slots.resize(slots.size() + 32, nullptr);
        }
        slots[bindIndex] = &buffers[i];
    }

    for (int i = 0; i <= maxBindIndex; i++) {
        const HgiShaderFunctionBufferDesc *p = slots[i];
        const HgiShaderSectionAttributeVector attributes = {
            HgiShaderSectionAttribute{"id", std::to_string(i)}
        };

        HgiMetalBufferShaderSection * section;
        if (p) {
            section =
                generator->CreateShaderSection<
                    HgiMetalBufferShaderSection>(
                        p->nameInShader,
                        _tokens->bufferBindings,
                        p->type,
                        p->binding,
                        p->writable,
                        attributes);
        }
        else {
            // Unused padding entry
            section =
                generator->CreateShaderSection<
                    HgiMetalBufferShaderSection>(
                        "_unused" + std::to_string(i),
                        attributes);
        }
        stageShaderSections.push_back(section);
    }
    return stageShaderSections;
}

HgiMetalShaderSectionPtrVector
ShaderStageData::AccumulateTextureBindings(
    const HgiShaderFunctionTextureDescVector &textures,
    HgiMetalShaderGenerator *generator)
{
    HgiMetalShaderSectionPtrVector stageShaderSections;

    for (size_t i = 0; i < textures.size(); i++) {
        //Create the sampler shader section
        const std::string &texName = textures[i].nameInShader;

        const HgiShaderSectionAttributeVector samplerAttributes = {
            HgiShaderSectionAttribute{"id", std::to_string(i)}
        };
        const HgiShaderSectionAttributeVector textureAttributes = {
            HgiShaderSectionAttribute{"id", std::to_string(i)}
        };
        
        //Shader section vector on the generator
        // owns all sections, point to it in the vector
        HgiMetalSamplerShaderSection * const samplerSection =
            generator->CreateShaderSection<HgiMetalSamplerShaderSection>(
                texName,
                _tokens->samplerBindings,
                textures[i].arraySize,
                samplerAttributes);

        //fx texturing struct depends on the sampler
        _samplerBindings.push_back(samplerSection);

        //Create the actual texture shader section
        HgiMetalTextureShaderSection * const textureSection =
            generator->CreateShaderSection<HgiMetalTextureShaderSection>(
                texName,
                _tokens->textureBindings,
                textureAttributes,
                samplerSection,
                textures[i].dimensions,
                textures[i].format,
                textures[i].textureType == HgiShaderTextureTypeArrayTexture,
                textures[i].arraySize,
                textures[i].textureType == HgiShaderTextureTypeShadowTexture,
                textures[i].writable,
                std::string());

        stageShaderSections.push_back(textureSection);
    }
    return stageShaderSections;
}

const HgiMetalShaderSectionPtrVector&
ShaderStageData::GetConstantParams() const
{
    return _constantParams;
}
const HgiMetalShaderSectionPtrVector&
ShaderStageData::GetInputs() const
{
    return _inputs;
}
const HgiMetalShaderSectionPtrVector&
ShaderStageData::GetOutputs() const
{
    return _outputs;
}
const HgiMetalShaderSectionPtrVector&
ShaderStageData::GetBufferBindings() const
{
    return _bufferBindings;
}
const HgiMetalShaderSectionPtrVector&
ShaderStageData::GetSamplerBindings() const
{
    return _samplerBindings;
}
const HgiMetalShaderSectionPtrVector&
ShaderStageData::GetTextureBindings() const
{
    return _textureBindings;
}

std::string _BuildOutputTypeName(const HgiMetalShaderStageEntryPoint &ep)
{
    const std::string &shortHandPrefix = ep.GetOutputShortHandPrefix();

    std::stringstream ss;
    ss << "MSL"
       << char(std::toupper(shortHandPrefix[0]))
       << shortHandPrefix[1]
       << "Outputs";
    return ss.str();
}

} // anonymous namespace

HgiMetalShaderStageEntryPoint::HgiMetalShaderStageEntryPoint(
      const ShaderStageData &stageData,
      HgiMetalShaderGenerator *generator,
      const std::string &outputShortHandPrefix,
      const std::string &scopePostfix,
      const std::string &entryPointStageName,
      const std::string &inputInstanceName,
      const std::string &entryPointAttributes)
    : _inputsGenericWrapper(stageData.inputsGenericWrapper),
      _outputShortHandPrefix(outputShortHandPrefix),
      _scopePostfix(scopePostfix),
      _entryPointStageName(entryPointStageName),
      _outputTypeName(_BuildOutputTypeName(*this)),
      _entryPointFunctionName(entryPointStageName + "EntryPoint"),
      _entryPointAttributes(entryPointAttributes),
      _inputInstanceName(inputInstanceName)
{
    _Init(
        stageData.GetConstantParams(),
        stageData.GetInputs(),
        stageData.GetOutputs(),
        stageData.GetBufferBindings(),
        stageData.GetSamplerBindings(),
        stageData.GetTextureBindings(),
        generator);
}

HgiMetalShaderStageEntryPoint::HgiMetalShaderStageEntryPoint(
    const ShaderStageData &stageData,
    HgiMetalShaderGenerator *generator,
    const std::string &outputShortHandPrefix,
    const std::string &scopePostfix,
    const std::string &entryPointStageName,
    const std::string &outputTypeName,
    const std::string &entryPointFunctionName,
    const std::string &entryPointAttributes)
  : _outputShortHandPrefix(outputShortHandPrefix),
    _scopePostfix(scopePostfix),
    _entryPointStageName(entryPointStageName),
    _outputTypeName(outputTypeName),
    _entryPointFunctionName(entryPointFunctionName)
{
    _Init(
        stageData.GetConstantParams(),
        stageData.GetInputs(),
        stageData.GetOutputs(),
        stageData.GetBufferBindings(),
        stageData.GetSamplerBindings(),
        stageData.GetTextureBindings(),
        generator);
}

const std::string&
HgiMetalShaderStageEntryPoint::GetInputsInstanceName() const
{
    return _inputInstanceName;
}

const std::string&
HgiMetalShaderStageEntryPoint::GetEntryPointFunctionName() const
{
    return _entryPointFunctionName;
}

const std::string&
HgiMetalShaderStageEntryPoint::GetOutputTypeName() const
{
    return _outputTypeName;
}

std::string
HgiMetalShaderStageEntryPoint::GetOutputInstanceName() const
{
    return GetOutputShortHandPrefix() + "Output";
}

const std::string&
HgiMetalShaderStageEntryPoint::GetScopeInstanceName() const
{
    static const std::string result = "scope";
    return result;
}

const std::string&
HgiMetalShaderStageEntryPoint::GetScopePostfix() const
{
    return _scopePostfix;
}

const std::string&
HgiMetalShaderStageEntryPoint::GetEntryPointStageName() const
{
    return _entryPointStageName;
}

const std::string&
HgiMetalShaderStageEntryPoint::GetEntryPointAttributes() const
{
    return _entryPointAttributes;
}

const std::string&
HgiMetalShaderStageEntryPoint::GetOutputShortHandPrefix() const
{
    return _outputShortHandPrefix;
}

std::string
HgiMetalShaderStageEntryPoint::GetConstantBufferTypeName() const
{
    const std::string &shortHandPrefix = GetOutputShortHandPrefix();

    std::stringstream ss;
    ss << "MSL"
       << char(std::toupper(shortHandPrefix[0]))
       << shortHandPrefix[1]
       << "Uniforms";
    return ss.str();
}

std::string
HgiMetalShaderStageEntryPoint::GetConstantBufferInstanceName() const
{
    return GetOutputShortHandPrefix() + "Uniforms";
}

std::string
HgiMetalShaderStageEntryPoint::GetScopeTypeName() const
{
    return "ProgramScope_" + GetScopePostfix();
}

std::string
HgiMetalShaderStageEntryPoint::GetInputsTypeName() const
{
    std::string inputInstance = GetInputsInstanceName();
    if(inputInstance.empty()) {
        return std::string();
    }
    inputInstance[0] = std::toupper(inputInstance[0]);
    return
        "MSL" + inputInstance;
};

std::string
HgiMetalShaderStageEntryPoint::GetBindingsBufferTypeName() const
{
    std::string inputInstance = _tokens->bufferBindings;
    inputInstance[0] = std::toupper(inputInstance[0]);
    return "MSL" + inputInstance;
};

std::string
HgiMetalShaderStageEntryPoint::GetBindingsSamplerTypeName() const
{
    std::string inputInstance = _tokens->samplerBindings;
    inputInstance[0] = std::toupper(inputInstance[0]);
    return "MSL" + inputInstance;
};

std::string
HgiMetalShaderStageEntryPoint::GetBindingsTextureTypeName() const
{
    std::string inputInstance = _tokens->textureBindings;
    inputInstance[0] = std::toupper(inputInstance[0]);
    return "MSL" + inputInstance;
};

HgiMetalParameterInputShaderSection*
HgiMetalShaderStageEntryPoint::GetParameters()
{
    return _parameters;
}

HgiMetalParameterInputShaderSection*
HgiMetalShaderStageEntryPoint::GetInputs()
{
    return _inputs;
}

HgiMetalStageOutputShaderSection*
HgiMetalShaderStageEntryPoint::GetOutputs()
{
    return _outputs;
}

void
HgiMetalShaderStageEntryPoint::_Init(
    const HgiMetalShaderSectionPtrVector &stageConstantBuffers,
    const HgiMetalShaderSectionPtrVector &stageInputs,
    const HgiMetalShaderSectionPtrVector &stageOutputs,
    const HgiMetalShaderSectionPtrVector &stageBufferBindings,
    const HgiMetalShaderSectionPtrVector &stageSamplerBindings,
    const HgiMetalShaderSectionPtrVector &stageTextureBindings,
    HgiMetalShaderGenerator *generator)
{
    static const std::string constIndex =
        "buffer(" + std::to_string(HgiMetalArgumentIndexConstants) + ")";
    static const std::string samplerIndex =
        "buffer(" + std::to_string(HgiMetalArgumentIndexSamplers) + ")";
    static const std::string textureIndex =
        "buffer(" + std::to_string(HgiMetalArgumentIndexTextures) + ")";
    static const std::string bufferIndex =
        "buffer(" + std::to_string(HgiMetalArgumentIndexBuffers) + ")";

    _parameters =
        _BuildStructInstance<HgiMetalParameterInputShaderSection>(
        GetConstantBufferTypeName(),
        GetConstantBufferInstanceName(),
        /* attribute = */ constIndex.c_str(),
        /* addressSpace = */ "const device",
        /* isPointer = */ true,
        /* members = */ stageConstantBuffers,
        generator);

    _inputs =
        _BuildStructInstance<HgiMetalParameterInputShaderSection>(
        GetInputsTypeName(),
        GetInputsInstanceName(),
        /* attribute = */ "stage_in",
        /* addressSpace = */ std::string(),
        /* isPointer = */ false,
        /* members = */ stageInputs,
        generator,
        _inputsGenericWrapper);

    _outputs =
        _BuildStructInstance<HgiMetalStageOutputShaderSection>(
        GetOutputTypeName(),
        GetOutputInstanceName(),
        /* attribute = */ std::string(),
        /* addressSpace = */ std::string(),
        /* isPointer = */ false,
        /* members = */ stageOutputs,
        generator);
    
    _bufferBindings =
        _BuildStructInstance<HgiMetalArgumentBufferInputShaderSection>(
        GetBindingsBufferTypeName(),
        _tokens->bufferBindings,
        /* attribute = */ bufferIndex.c_str(),
        /* addressSpace = */ "const device",
        /* isPointer = */ true,
        /* members = */ stageBufferBindings,
        generator);

    _samplerBindings =
        _BuildStructInstance<HgiMetalArgumentBufferInputShaderSection>(
        GetBindingsSamplerTypeName(),
        _tokens->samplerBindings,
        /* attribute = */ samplerIndex.c_str(),
        /* addressSpace = */ "const device",
        /* isPointer = */ true,
        /* members = */ stageSamplerBindings,
        generator);

    _textureBindings =
        _BuildStructInstance<HgiMetalArgumentBufferInputShaderSection>(
        GetBindingsTextureTypeName(),
        _tokens->textureBindings,
        /* attribute = */ textureIndex.c_str(),
        /* addressSpace = */ "const device",
        /* isPointer = */ true,
        /* members = */ stageTextureBindings,
        generator);
}

//Instantiate special keyword shader sections based on the given descriptor
void HgiMetalShaderGenerator::_BuildKeywordInputShaderSections(
    const HgiShaderFunctionDesc &descriptor)
{
    //possible metal attributes on shader inputs.
    // Map from descriptor to Metal
    std::unordered_map<std::string, std::string> roleIndexM {
       {HgiShaderKeywordTokens->hdGlobalInvocationID, "thread_position_in_grid"}
    };

    const std::vector<HgiShaderFunctionParamDesc> &inputs =
        descriptor.stageInputs;
    for (size_t i = 0; i < inputs.size(); ++i) {
        const HgiShaderFunctionParamDesc &p(inputs[i]);

        std::string msl_attrib;
        if(_GetBuiltinKeyword(p, &msl_attrib)) {
            const std::string &keywordName = p.nameInShader;

            const HgiShaderSectionAttributeVector attributes = {
                HgiShaderSectionAttribute{msl_attrib, "" }};

            //Shader section vector on the generator
            // owns all sections, point to it in the vector
            CreateShaderSection<HgiMetalKeywordInputShaderSection>(
                keywordName,
                p.type,
                attributes);
        }
    }
}

void
_BuildTessAttribute(
        std::stringstream &ss,
        const HgiShaderFunctionTessellationDesc &tessDesc)
{
    ss << "[[patch(";
    switch (tessDesc.patchType) {
        case HgiShaderFunctionTessellationDesc::PatchType::Triangles:
            ss << "triangle, ";
            break;
        case HgiShaderFunctionTessellationDesc::PatchType::Quads:
            ss << "quad, ";
            break;
        default:
            TF_CODING_ERROR("Unknown patch type");
            break;
    }
    ss << tessDesc.numVertsPerPatchIn << ")]]";
}

void
_BuildFragmentAttribute(
        std::stringstream &ss,
        const HgiShaderFunctionFragmentDesc &fragmentDesc)
{
    if (fragmentDesc.earlyFragmentTests) {
        ss << "[[early_fragment_tests]]\n";
    }
}

void
_BuildComputeAttribute(
        std::stringstream &ss,
        const HgiShaderFunctionComputeDesc &computeDesc)
{
    if (computeDesc.localSize[0] > 0 &&
        computeDesc.localSize[1] > 0 &&
        computeDesc.localSize[2] > 0) {
        ss << "[[max_total_threads_per_threadgroup("
           << computeDesc.localSize[0] << " * "
           << computeDesc.localSize[1] << " * "
           << computeDesc.localSize[2] << ")]]\n";
    }
}

std::unique_ptr<HgiMetalShaderStageEntryPoint>
HgiMetalShaderGenerator::_BuildShaderStageEntryPoints(
    const HgiShaderFunctionDesc &descriptor)
{
    _BuildKeywordInputShaderSections(descriptor);

    //Create differing shader function signature based on stage
    const ShaderStageData stageData(descriptor, this);

    std::stringstream functionAttributesSS = std::stringstream();
    
    switch (descriptor.shaderStage) {
        case HgiShaderStageVertex: {
            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                        stageData,
                        this,
                        "vsInput",
                        "vsInput",
                        "vertex",
                        "vsInput",
                        functionAttributesSS.str());
        }
        case HgiShaderStageFragment: {
            _BuildFragmentAttribute(functionAttributesSS,
                                    descriptor.fragmentDescriptor);

            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                        stageData,
                        this,
                        "fs",
                        "Frag",
                        "fragment",
                        "vsOutput",
                        functionAttributesSS.str());
        }
        case HgiShaderStageCompute: {
            _BuildComputeAttribute(functionAttributesSS,
                                   descriptor.computeDescriptor);

            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                        stageData,
                        this,
                        "cs",
                        "Compute",
                        "kernel",
                        "void",
                        "computeEntryPoint",
                        functionAttributesSS.str());
        }
        case HgiShaderStagePostTessellationVertex: {
            _BuildTessAttribute(functionAttributesSS,
                                descriptor.tessellationDescriptor);

            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                            stageData,
                            this,
                            "tv",
                            "TessVert",
                            "vertex",
                            "tvInput",
                            functionAttributesSS.str());
        }
        case HgiShaderStagePostTessellationControl: {
            _BuildTessAttribute(functionAttributesSS,
                                descriptor.tessellationDescriptor);
            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                            stageData,
                            this,
                            "ptc",
                            "TessControl",
                            "vertex",
                            "tcInput",
                            functionAttributesSS.str());
        }
        default: {
            TF_CODING_ERROR("Unknown shader stage");
            return nullptr;
        }
    }
}

HgiMetalShaderGenerator::HgiMetalShaderGenerator(
    const HgiShaderFunctionDesc &descriptor,
    id<MTLDevice> device)
  : HgiShaderGenerator(descriptor)
  , _generatorShaderSections(_BuildShaderStageEntryPoints(descriptor))
{
    for (const auto &member: descriptor.stageGlobalMembers) {
        HgiShaderSectionAttributeVector attrs;
        CreateShaderSection<
            HgiMetalMemberShaderSection>(
                member.nameInShader,
                member.type,
                attrs,
                member.arraySize);
    }
    std::stringstream macroSection;
    macroSection << _GetHeader(device, descriptor.shaderStage);
    if (_IsTessFunction(descriptor)) {
        macroSection << "#define VERTEX_CONTROL_POINTS_PER_PATCH "
        << descriptor.tessellationDescriptor.numVertsPerPatchIn
        << "\n";
    }
    
    CreateShaderSection<HgiMetalMacroShaderSection>(
        macroSection.str(),
        "Headers");

}

HgiMetalShaderGenerator::~HgiMetalShaderGenerator() = default;

void HgiMetalShaderGenerator::_Execute(std::ostream &ss)
{
    HgiMetalShaderSectionUniquePtrVector * const shaderSections =
        GetShaderSections();

    ss << "\n// //////// Global Macros ////////\n";
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitGlobalMacros(ss);
    }

    ss << _GetShaderCodeDeclarations();

    ss << "\n// //////// Global Member Declarations ////////\n";
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitGlobalMemberDeclarations(ss);
    }

    //generate scope area in metal.
    //We create a class that wraps the main shader function, and to simulate
    //global space in metal which it has not by default, we put all
    //glslfx global members into a Scope struct, and host the global members
    //as members of that instance
    ss << "struct " << _generatorShaderSections->GetScopeTypeName() << " { \n";

    // Metal extends the global scope into a "scope" embedder,
    // which simulates a global scope for some member variables
    ss << "\n// //////// Scope Structs ////////\n";
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitScopeStructs(ss);
    }
    ss << "\n// //////// Scope Member Declarations ////////\n";
    if (this->_GetShaderStage() == HgiShaderStageFragment) {
        ss << "bool discarded_fragment;\n";
    }
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitScopeMemberDeclarations(ss);
    }
    ss << "\n// //////// Scope Function Definitions ////////\n";
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitScopeFunctionDefinitions(ss);
    }

    //constructor
    ss << _generatorShaderSections->GetScopeTypeName() << "(\n";
    bool firstParam = true;
    bool hasContructorParams = false;
    ss << "\n// //////// Scope Constructor Declarations ////////\n";
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        std::stringstream paramDecl;
        if (section->VisitScopeConstructorDeclarations(paramDecl)) {
            if(!firstParam) {
                ss << ",\n";
            }
            else {
                firstParam = false;
            }
            ss << paramDecl.str();
            hasContructorParams = true;
        }
    }
    ss << ")";
    
    if (hasContructorParams) {
        ss << ":\n";
        firstParam = true;
        ss << "\n// //////// Scope Constructor Initialization ////////\n";
        for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
            std::stringstream paramDecl;
            if (section->VisitScopeConstructorInitialization(paramDecl)) {
                if(!firstParam) {
                    ss << ",\n";
                }
                else {
                    firstParam = false;
                }
                ss << paramDecl.str();
            }
        }
    }
    ss << "{};\n\n";
    
    ss << _GetShaderCode();
    ss << "};\n\n";

    //write out the entry point signature
    HgiMetalStageOutputShaderSection* const outputs =
        _generatorShaderSections->GetOutputs();
    std::stringstream returnSS;
    if (outputs &&
        (_GetShaderStage() != HgiShaderStagePostTessellationControl)) {
        const HgiMetalStructTypeDeclarationShaderSection* const decl =
            outputs->GetStructTypeDeclaration();
        decl->WriteIdentifier(returnSS);
    }
    else {
        //handle compute
        returnSS << "void";
    }

    ss << _generatorShaderSections->GetEntryPointAttributes();

    ss << _generatorShaderSections->GetEntryPointStageName();
    ss << " " << returnSS.str() << " "
       << _generatorShaderSections->GetEntryPointFunctionName() << "(\n";

    // Pass in all parameters declared by interested code sections into the
    // entry point of the shader
    firstParam = true;
    ss << "\n// //////// Entry Point Parameter Declarations ////////\n";
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        std::stringstream paramDecl;
        if (section->VisitEntryPointParameterDeclarations(paramDecl)) {
            if(!firstParam) {
                ss << ",\n";
            }
            else {
                firstParam = false;
            }
            ss << paramDecl.str();
        }
    }
    ss <<"){\n";
    ss << _generatorShaderSections->GetScopeTypeName() << " "
       << _generatorShaderSections->GetScopeInstanceName();
    
    if (hasContructorParams) {
        ss << "(\n";
        firstParam = true;
        ss << "\n// //////// Scope Constructor Instantiation ////////\n";
        for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
            std::stringstream paramDecl;
            if (section->VisitScopeConstructorInstantiation(paramDecl)) {
                if(!firstParam) {
                    ss << ",\n";
                }
                else {
                    firstParam = false;
                }
                ss << paramDecl.str();
            }
        }
        ss << ")";
    }
    ss << ";\n";

    // Execute all code that hooks into the entry point function
    ss << "\n// //////// Entry Point Function Executions ////////\n";
    if (this->_GetShaderStage() == HgiShaderStageFragment) {
        ss << _generatorShaderSections->GetScopeInstanceName() 
           << ".discarded_fragment = false;\n";
    }
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        if (section->VisitEntryPointFunctionExecutions(
                ss, _generatorShaderSections->GetScopeInstanceName())) {
            ss << "\n";
        }
    }
    if (this->_GetShaderStage() == HgiShaderStageFragment) {
        ss << "if (" << _generatorShaderSections->GetScopeInstanceName() 
           << ".discarded_fragment)\n";
        ss << "{\n";
        if (outputs) {
            ss << "    return {};\n";
        } else {
            ss << "    return;\n";
        }
        ss << "}\n";
    }
    //return the instance of the shader entrypoint output type
    if (outputs &&
        (_GetShaderStage() != HgiShaderStagePostTessellationControl)) {
        const std::string outputInstanceName =
                _generatorShaderSections->GetOutputInstanceName();
        ss << "return " << outputInstanceName << ";\n";
    }
    else {
        ss << _generatorShaderSections->GetScopeInstanceName() << ".main();\n";
    }
    ss << "}\n";
}

HgiMetalShaderSectionUniquePtrVector*
HgiMetalShaderGenerator::GetShaderSections()
{
    return &_shaderSections;
}


PXR_NAMESPACE_CLOSE_SCOPE
