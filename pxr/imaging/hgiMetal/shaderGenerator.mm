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
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

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
        bool iterateAttrs=false);

    const HgiMetalShaderSectionPtrVector& GetConstantParams() const;
    const HgiMetalShaderSectionPtrVector& GetInputs() const;
    const HgiMetalShaderSectionPtrVector& GetOutputs() const;

private:
    ShaderStageData() = delete;
    ShaderStageData & operator=(const ShaderStageData&) = delete;
    ShaderStageData(const ShaderStageData&) = delete;

    const HgiMetalShaderSectionPtrVector _constantParams;
    const HgiMetalShaderSectionPtrVector _inputs;
    const HgiMetalShaderSectionPtrVector _outputs;
};

template<typename T>
T* _BuildStructInstance(
    const std::string &typeName,
    const std::string &instanceName,
    const std::string &attribute,
    const std::string &addressSpace,
    const bool isPointer,
    const HgiMetalShaderSectionPtrVector &members,
    HgiMetalShaderGenerator *generator)
{
    //If it doesn't have any members, don't declare an empty struct instance
    if(typeName.empty() || members.empty()) {
        return nullptr;
    }

    HgiMetalStructTypeDeclarationShaderSection * const section =
        generator->CreateShaderSection<
            HgiMetalStructTypeDeclarationShaderSection>(
                typeName,
                members);

    const HgiShaderSectionAttributeVector attributes = {
        HgiShaderSectionAttribute{attribute, ""}};

    return generator->CreateShaderSection<T>(
        instanceName,
        attributes,
        addressSpace,
        isPointer,
        section);
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
          const std::string &entryPointFunctionName);
    
    HgiMetalShaderStageEntryPoint(
          const ShaderStageData &stageData,
          HgiMetalShaderGenerator *generator,
          const std::string &outputShortHandPrefix,
          const std::string &scopePostfix,
          const std::string &entryPointStageName,
          const std::string &inputInstanceName);

    const std::string& GetOutputShortHandPrefix() const;
    const std::string& GetScopePostfix() const;
    const std::string& GetEntryPointStageName() const;
    const std::string& GetEntryPointFunctionName() const;
    const std::string& GetOutputTypeName() const;
    const std::string& GetInputsInstanceName() const;
    
    std::string GetOutputInstanceName() const;
    const std::string& GetScopeInstanceName() const;
    std::string GetConstantBufferTypeName() const;
    std::string GetConstantBufferInstanceName() const;
    std::string GetScopeTypeName() const;
    std::string GetInputsTypeName() const;
    HgiMetalArgumentBufferInputShaderSection* GetParameters();
    HgiMetalArgumentBufferInputShaderSection* GetInputs();
    HgiMetalStageOutputShaderSection* GetOutputs();

private:
    void _Init(
        const HgiMetalShaderSectionPtrVector &stageConstantBuffers,
        const HgiMetalShaderSectionPtrVector &stageInputs,
        const HgiMetalShaderSectionPtrVector &stageOutputs,
        HgiMetalShaderGenerator *generator);

    HgiMetalShaderStageEntryPoint & operator=(
        const HgiMetalShaderStageEntryPoint&) = delete;
    HgiMetalShaderStageEntryPoint(
        const HgiMetalShaderStageEntryPoint&) = delete;

    //Owned by and stored in shadersections
    HgiMetalArgumentBufferInputShaderSection* _parameters;
    HgiMetalArgumentBufferInputShaderSection* _inputs;
    HgiMetalStageOutputShaderSection* _outputs;
    const std::string _outputShortHandPrefix;
    const std::string _scopePostfix;
    const std::string _entryPointStageName;
    const std::string _outputTypeName;
    const std::string _entryPointFunctionName;
    const std::string _inputInstanceName;
};

namespace {

//This is used by the macro blob, basically this is dumped on top
//of the generated shader
const char *
_GetPackedTypeDefinitions()
{
    return
    "#define hd_ivec2 packed_int2\n"
    "#define hd_ivec3 packed_int3\n"
    "#define hd_vec2 packed_float2\n"
    "#define hd_dvec2 packed_float2\n"
    "#define hd_vec3 packed_float3\n"
    "#define hd_dvec3 packed_float3\n"
    "#define REF(space,type) space type &\n"
    "struct hd_mat3  { float m00, m01, m02,\n"
    "                        m10, m11, m12,\n"
    "                        m20, m21, m22;\n"
    "                    hd_mat3(float _00, float _01, float _02,\n"
    "                            float _10, float _11, float _12,\n"
    "                            float _20, float _21, float _22)\n"
    "                              : m00(_00), m01(_01), m02(_02)\n"
    "                              , m10(_10), m11(_11), m12(_12)\n"
    "                              , m20(_20), m21(_21), m22(_22) {}\n"
    "                };\n"
    "struct hd_dmat3  { float m00, m01, m02,\n"
    "                         m10, m11, m12,\n"
    "                         m20, m21, m22;\n"
    "                    hd_dmat3(float _00, float _01, float _02,\n"
    "                            float _10, float _11, float _12,\n"
    "                            float _20, float _21, float _22)\n"
    "                              : m00(_00), m01(_01), m02(_02)\n"
    "                              , m10(_10), m11(_11), m12(_12)\n"
    "                              , m20(_20), m21(_21), m22(_22) {}\n"
    "                };\n"
    "#define hd_ivec3_get(v) packed_int3(v)\n"
    "#define hd_vec3_get(v)  packed_float3(v)\n"
    "#define hd_dvec3_get(v) packed_float3(v)\n"
    "mat3  hd_mat3_get(hd_mat3 v)   { return mat3(v.m00, v.m01, v.m02,\n"
    "                                             v.m10, v.m11, v.m12,\n"
    "                                             v.m20, v.m21, v.m22); }\n"
    "mat3  hd_mat3_get(mat3 v)      { return v; }\n"
    "dmat3 hd_dmat3_get(hd_dmat3 v) { return dmat3(v.m00, v.m01, v.m02,\n"
    "                                              v.m10, v.m11, v.m12,\n"
    "                                              v.m20, v.m21, v.m22); }\n"
    "dmat3 hd_dmat3_get(dmat3 v)    { return v; }\n"
    "hd_ivec3 hd_ivec3_set(hd_ivec3 v) { return v; }\n"
    "hd_ivec3 hd_ivec3_set(ivec3 v)    { return v; }\n"
    "hd_vec3 hd_vec3_set(hd_vec3 v)    { return v; }\n"
    "hd_vec3 hd_vec3_set(vec3 v)       { return v; }\n"
    "hd_dvec3 hd_dvec3_set(hd_dvec3 v) { return v; }\n"
    "hd_dvec3 hd_dvec3_set(dvec3 v)    { return v; }\n"
    "hd_mat3  hd_mat3_set(hd_mat3 v)   { return v; }\n"
    "hd_mat3  hd_mat3_set(mat3 v)      {\n"
    "   return hd_mat3(v[0][0], v[0][1], v[0][2],\n"
    "                  v[1][0], v[1][1], v[1][2],\n"
    "                  v[2][0], v[2][1], v[2][2]); }\n"
    "hd_dmat3 hd_dmat3_set(hd_dmat3 v) { return v; }\n"
    "hd_dmat3 hd_dmat3_set(dmat3 v)    {\n"
    "   return hd_dmat3(v[0][0], v[0][1], v[0][2],\n"
    "                   v[1][0], v[1][1], v[1][2],\n"
    "                   v[2][0], v[2][1], v[2][2]); }\n"
    "int hd_int_get(int v)          { return v; }\n"
    "int hd_int_get(ivec2 v)        { return v[0]; }\n"
    "int hd_int_get(ivec3 v)        { return v[0]; }\n"
    "int hd_int_get(ivec4 v)        { return v[0]; }\n"

    // udim helper function
    "vec3 hd_sample_udim(vec2 v) {\n"
    "vec2 vf = floor(v);\n"
    "return vec3(v.x-vf.x, v.y-vf.y, clamp(vf.x, 0.0, 10.0) + 10.0 * vf.y);\n"
    "}\n"

    // -------------------------------------------------------------------
    // Packed HdType implementation.

    "struct packedint1010102 { int x:10, y:10, z:10, w:2; };\n"
    "#define packed_2_10_10_10 int\n"
    "vec4 hd_vec4_2_10_10_10_get(int v) {\n"
    "    packedint1010102 pi = *(thread packedint1010102*)&v;\n"
    "    return vec4(vec3(pi.x, pi.y, pi.z) / 511.0f, pi.w); }\n"
    "int hd_vec4_2_10_10_10_set(vec4 v) {\n"
    "    packedint1010102 pi;\n"
    "    pi.x = v.x * 511.0; pi.y = v.y * 511.0; pi.z = v.z * 511.0; pi.w = 0;\n"
    "    return *(thread int*)&pi;\n"
    "}\n"

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
_ComputeHeader(id<MTLDevice> device)
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
            << "using namespace metal;\n";

    header  << "#define double float\n"
            << "#define vec2 float2\n"
            << "#define vec3 float3\n"
            << "#define vec4 float4\n"
            << "#define mat3 float3x3\n"
            << "#define mat4 float4x4\n"
            << "#define ivec2 int2\n"
            << "#define ivec3 int3\n"
            << "#define ivec4 int4\n"
            << "#define bvec2 bool2\n"
            << "#define bvec3 bool3\n"
            << "#define bvec4 bool4\n"
            << "#define dvec2 float2\n"
            << "#define dvec3 float3\n"
            << "#define dvec4 float4\n"
            << "#define dmat3 float3x3\n"
            << "#define dmat4 float4x4\n";

    // XXX: this macro is still used in GlobalUniform.
    header  << "#define MAT4 mat4\n";

    // a trick to tightly pack vec3 into SSBO/UBO.
    header  << _GetPackedTypeDefinitions();

    header  << "#define in /*in*/\n"
    << "#define discard discard_fragment();\n"
    << "#define radians(d) (d * 0.01745329252)\n"
    << "#define noperspective /*center_no_perspective MTL_FIXME*/\n"
    << "#define dFdx    dfdx\n"
    << "#define dFdy    dfdy\n"

    << "#define lessThan(a, b) ((a) < (b))\n"
    << "#define lessThanEqual(a, b) ((a) <= (b))\n"
    << "#define greaterThan(a, b) ((a) > (b))\n"
    << "#define greaterThanEqual(a, b) ((a) >= (b))\n"
    << "#define equal(a, b) ((a) == (b))\n"
    << "#define notEqual(a, b) ((a) != (b))\n"

    << "template <typename T>\n"
    << "T mod(T y, T x) { return fmod(y, x); }\n\n"
    << "template <typename T>\n"
    << "T atan(T y, T x) { return atan2(y, x); }\n\n"
    << "template <typename T>\n"
    << "T bitfieldReverse(T x) { return reverse_bits(x); }\n\n"
    << "template <typename T>\n"
    << "ivec2 imageSize(T texture) {\n"
    << "    return ivec2(texture.get_width(), texture.get_height());\n"
    << "}\n\n"

    << "template <typename T>\n"
    << "ivec2 textureSize(T texture, int lod) {\n"
    << "    return ivec2(texture.get_width(lod), texture.get_height(lod));\n"
    << "}\n\n"

    << "constexpr sampler texelSampler(address::clamp_to_edge,\n"
    << "                               filter::linear);\n";

    // wrapper for type float and int to deal with .x accessors and the
    // like that are valid in GLSL
    header  << "struct wrapped_float {\n"
            << "    union {\n"
            << "        float x, xx, xxx, xxxx, y, z, w;\n"
            << "        float r, rr, rrr, rrrr, g, b, a;\n"
            << "    };\n"
            << "    wrapped_float(float _x) { x = _x;}\n"
            << "    operator float () {\n"
            << "        return x;\n"
            << "    }\n"
            << "};\n";

    header  << "struct wrapped_int {\n"
            << "    union {\n"
            << "        int x, xx, xxx, xxxx, y, z, w;\n"
            << "        int r, rr, rrr, rrrr, g, b, a;\n"
            << "    };\n"
            << "    wrapped_int(int _x) { x = _x;}\n"
            << "    operator int () {\n"
            << "        return x;\n"
            << "    }\n"
            << "};\n";

    return header.str();
}

std::string const&
_GetHeader(id<MTLDevice> device)
{
    // This assumes that there is only ever one MTLDevice.
    static std::string header = _ComputeHeader(device);
    return header;
}

ShaderStageData::ShaderStageData(
    const HgiShaderFunctionDesc &descriptor,
    HgiMetalShaderGenerator *generator)
  : _constantParams(
          AccumulateParams(
              descriptor.constantParams,
              generator))
  , _inputs(
          AccumulateParams(
              descriptor.stageInputs,
              generator,
              descriptor.shaderStage == HgiShaderStageVertex))
  , _outputs(
          AccumulateParams(
              descriptor.stageOutputs,
              generator))
{
}

//Convert ShaderFunctionParamDescs into shader sections
HgiMetalShaderSectionPtrVector 
ShaderStageData::AccumulateParams(
    const HgiShaderFunctionParamDescVector &params,
    HgiMetalShaderGenerator *generator,
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

            const HgiShaderSectionAttributeVector attributes = {
                HgiShaderSectionAttribute{p.role, indexAsStr} };

            HgiMetalMemberShaderSection * const section =
                generator->CreateShaderSection<
                    HgiMetalMemberShaderSection>(
                        p.nameInShader,
                        p.type,
                        attributes);
            stageShaderSections.push_back(section);
        }
    } else {
        for (size_t i = 0; i < params.size(); i++) {
            const HgiShaderFunctionParamDesc &p = params[i];
            //For metal, the role is the actual attribute so far
            const HgiShaderSectionAttributeVector attributes = {
                HgiShaderSectionAttribute{"attribute", std::to_string(i)}};

            HgiMetalMemberShaderSection * const section =
                generator->CreateShaderSection<
                    HgiMetalMemberShaderSection>(
                        p.nameInShader,
                        p.type,
                        attributes);
            stageShaderSections.push_back(section);
        }
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
      const std::string &inputInstanceName)
    : _outputShortHandPrefix(outputShortHandPrefix),
      _scopePostfix(scopePostfix),
      _entryPointStageName(entryPointStageName),
      _outputTypeName(_BuildOutputTypeName(*this)),
      _entryPointFunctionName(entryPointStageName + "EntryPoint"),
      _inputInstanceName(inputInstanceName)
{
    _Init(
        stageData.GetConstantParams(),
        stageData.GetInputs(),
        stageData.GetOutputs(),
        generator);
}

HgiMetalShaderStageEntryPoint::HgiMetalShaderStageEntryPoint(
    const ShaderStageData &stageData,
    HgiMetalShaderGenerator *generator,
    const std::string &outputShortHandPrefix,
    const std::string &scopePostfix,
    const std::string &entryPointStageName,
    const std::string &outputTypeName,
    const std::string &entryPointFunctionName)
  : _outputShortHandPrefix(outputShortHandPrefix),
    _scopePostfix(scopePostfix),
    _entryPointStageName(entryPointStageName),
    _outputTypeName(outputTypeName),
    _entryPointFunctionName(entryPointFunctionName),
    _inputInstanceName()
{
    _Init(
        stageData.GetConstantParams(),
        stageData.GetInputs(),
        stageData.GetOutputs(),
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
    return "MSL" + inputInstance;
};


HgiMetalArgumentBufferInputShaderSection*
HgiMetalShaderStageEntryPoint::GetParameters()
{
    return _parameters;
}

HgiMetalArgumentBufferInputShaderSection*
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
    HgiMetalShaderGenerator *generator)
{
    _parameters =
        _BuildStructInstance<HgiMetalArgumentBufferInputShaderSection>(
        GetConstantBufferTypeName(),
        GetConstantBufferInstanceName(),
        /* attribute = */ "buffer(0)",
        /* addressSpace = */ "const device",
        /* isPointer = */ true,
        /* members = */ stageConstantBuffers,
        generator);

    _inputs =
        _BuildStructInstance<HgiMetalArgumentBufferInputShaderSection>(
        GetInputsTypeName(),
        GetInputsInstanceName(),
        /* attribute = */ "stage_in",
        /* addressSpace = */ std::string(),
        /* isPointer = */ false,
        /* members = */ stageInputs,
        generator);

    _outputs =
        _BuildStructInstance<HgiMetalStageOutputShaderSection>(
        GetOutputTypeName(),
        GetOutputInstanceName(),
        /* attribute = */ std::string(),
        /* addressSpace = */ std::string(),
        /* isPointer = */ false,
        /* members = */ stageOutputs,
        generator);
}

//Instantiate texture shader sections based on the given descriptor
void HgiMetalShaderGenerator::_BuildTextureShaderSections(
    const HgiShaderFunctionDesc &descriptor)
{
    HgiMetalShaderSectionPtrVector structMembers;
    const std::vector<HgiShaderFunctionTextureDesc> &textures =
        descriptor.textures;
    for (size_t i = 0; i < textures.size(); ++i) {
        //Create the sampler shader section
        const std::string &texName = textures[i].nameInShader;
        
        const HgiShaderSectionAttributeVector samplerAttributes = {
            HgiShaderSectionAttribute{"sampler", std::to_string(i)}};

        //Shader section vector on the generator
        // owns all sections, point to it in the vector
        HgiMetalSamplerShaderSection * const samplerSection =
            CreateShaderSection<HgiMetalSamplerShaderSection>(
                texName, samplerAttributes);

        //fx texturing struct depends on the sampler
        structMembers.push_back(samplerSection);

        const HgiShaderSectionAttributeVector textureAttributes = {
            HgiShaderSectionAttribute{"texture", std::to_string(i) } };

        //Create the actual texture shader section
        HgiMetalTextureShaderSection * const textureSection =
            CreateShaderSection<HgiMetalTextureShaderSection>(
                texName,
                textureAttributes,
                samplerSection,
                std::string());

        //fx texturing struct depends on the sampler
        structMembers.push_back(textureSection);
    }

    HgiMetalStructTypeDeclarationShaderSection * const structSection =
        CreateShaderSection<HgiMetalStructTypeDeclarationShaderSection>(
            "MSLFsTexturing", structMembers);

    CreateShaderSection<HgiMetalArgumentBufferInputShaderSection>(
        "fsTexturing",
        HgiShaderSectionAttributeVector{},
        std::string(),
        false,
        structSection);
}

std::unique_ptr<HgiMetalShaderStageEntryPoint>
HgiMetalShaderGenerator::_BuildShaderStageEntryPoints(
    const HgiShaderFunctionDesc &descriptor)
{
    if(!descriptor.textures.empty()) {
        _BuildTextureShaderSections(descriptor);
    }

    //Create differing shader function signature based on stage
    const ShaderStageData stageData(descriptor, this);
    
    switch (descriptor.shaderStage) {
        case HgiShaderStageVertex: {
            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                        stageData,
                        this,
                        "vsInput",
                        "vsInput",
                        "vertex",
                        "vsInput");
        }
        case HgiShaderStageFragment: {
            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                        stageData,
                        this,
                        "fs",
                        "Frag",
                        "fragment",
                        "vsOutput");
        }
        case HgiShaderStageCompute: {
            return std::make_unique
                    <HgiMetalShaderStageEntryPoint>(
                        stageData,
                        this,
                        "cs",
                        "Compute",
                        "kernel",
                        "void",
                        "computeEntryPoint");
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
    CreateShaderSection<HgiMetalMacroShaderSection>(
        _GetHeader(device),
        "Headers");
}

HgiMetalShaderGenerator::~HgiMetalShaderGenerator() = default;

void HgiMetalShaderGenerator::_Execute(
    std::ostream &ss, const std::string &originalShaderCode)
{
    HgiMetalShaderSectionUniquePtrVector * const shaderSections =
        GetShaderSections();
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitGlobalMacros(ss);
        ss << "\n";
    }
    
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitGlobalMemberDeclarations(ss);
        ss << "\n";
    }

    //generate scope area in metal.
    //We create a class that wraps the main shader function, and to simulate
    //global space in metal which it has not by default, we put all
    //glslfx global members into a Scope struct, and host the global members
    //as members of that instance
    ss << "struct " << _generatorShaderSections->GetScopeTypeName() << " { \n";

    //Metal extends the global scope into a "scope" embedder,
    //which simulates a global scope for some member variables
    for(const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitScopeStructs(ss);
    }
    for(const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitScopeMemberDeclarations(ss);
    }
    for(const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        section->VisitScopeFunctionDefinitions(ss);
    }

    ss << originalShaderCode;
    ss << "};\n\n";

    //write out the entry point signature
    std::stringstream returnSS;
    if (HgiMetalStageOutputShaderSection* const outputs =
                        _generatorShaderSections->GetOutputs()) {
        const HgiMetalStructTypeDeclarationShaderSection* const decl =
            outputs->GetStructTypeDeclaration();
        decl->WriteIdentifier(returnSS);
    }
    else {
        //handle compute
        returnSS << "void";
    }
    ss << _generatorShaderSections->GetEntryPointStageName();
    ss << " " << returnSS.str() << " "
       << _generatorShaderSections->GetEntryPointFunctionName() << "(\n";

    //Pass in all parameters declared by interested code sections into the
    //entry point of the shader
    bool firstParam = true;
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
       << _generatorShaderSections->GetScopeInstanceName() << ";\n";
    
    //Execute all code that hooks into the entry point function
    for (const HgiMetalShaderSectionUniquePtr &section : *shaderSections) {
        if (section->VisitEntryPointFunctionExecutions(
                ss, _generatorShaderSections->GetScopeInstanceName())) {
            ss << "\n";
        }
    }
    //return the instance of the shader entrypoint output type
    const std::string outputInstanceName =
            _generatorShaderSections->GetOutputInstanceName();
    if(!outputInstanceName.empty())
    {
        ss << "return " << outputInstanceName << ";\n";
    }
    ss << "}\n";
}

HgiMetalShaderSectionUniquePtrVector*
HgiMetalShaderGenerator::GetShaderSections()
{
    return &_shaderSections;
}

PXR_NAMESPACE_CLOSE_SCOPE
