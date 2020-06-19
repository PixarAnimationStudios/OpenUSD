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
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"

#include "pxr/base/arch/defines.h"

PXR_NAMESPACE_OPEN_SCOPE

static const char *
_GetPackedTypeDefinitions()
{
    return
    "#define hd_ivec2 packed_int2\n"
    "#define hd_ivec3 packed_int3\n"
    "#define hd_vec2 packed_float2\n"
    "#define hd_dvec2 packed_float2\n"
    "#define hd_vec3 packed_float3\n"
    "#define hd_dvec3 packed_float3\n"
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

static std::string
_ComputeHeader(id<MTLDevice> device)
{
    static std::stringstream header;

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

static std::string const&
_GetHeader(id<MTLDevice> device)
{
    static std::string header = _ComputeHeader(device);
    return header;
}

HgiMetalShaderFunction::HgiMetalShaderFunction(
    HgiMetal *hgi,
    HgiShaderFunctionDesc const& desc)
    : HgiShaderFunction(desc)
    , _shaderId(nil)
{
    id<MTLDevice> device = hgi->GetPrimaryDevice();

    std::string source = _GetHeader(device) + desc.shaderCode;

    MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
    options.fastMathEnabled = YES;
    options.languageVersion = MTLLanguageVersion2_1;
    options.preprocessorMacros = @{
        @"ARCH_GFX_METAL":@1,
    };
    
    NSError *error = NULL;
    id<MTLLibrary> library =
        [hgi->GetPrimaryDevice() newLibraryWithSource:@(source.c_str())
                                              options:options
                                                error:&error];

    NSString *entryPoint = nullptr;
    switch (_descriptor.shaderStage) {
        case HgiShaderStageVertex:
            entryPoint = @"vertexEntryPoint";
            break;
        case HgiShaderStageFragment:
            entryPoint = @"fragmentEntryPoint";
            break;
        case HgiShaderStageCompute:
            entryPoint = @"computeEntryPoint";
            break;
        case HgiShaderStageTessellationControl:
        case HgiShaderStageTessellationEval:
        case HgiShaderStageGeometry:
            TF_CODING_ERROR("Todo: Unsupported shader stage");
            break;
    }
    
    // Load the function into the library
    _shaderId = [library newFunctionWithName:entryPoint];
    if (!_shaderId) {
        NSString *err = [error localizedDescription];
        TF_WARN("Failed to compile shader: \n%s",
                [err UTF8String]);
        TF_WARN("%s", source.c_str());
    }
    else {
        HGIMETAL_DEBUG_LABEL(_shaderId, _descriptor.debugName.c_str());
    }

    _descriptor.shaderCode = nullptr;
    [library release];
}

HgiMetalShaderFunction::~HgiMetalShaderFunction()
{
    [_shaderId release];
    _shaderId = nil;
}

bool
HgiMetalShaderFunction::IsValid() const
{
    return _errors.empty();
}

std::string const&
HgiMetalShaderFunction::GetCompileErrors()
{
    return _errors;
}

size_t
HgiMetalShaderFunction::GetByteSizeOfResource() const
{
    return 0;
}

uint64_t
HgiMetalShaderFunction::GetRawResource() const
{
    return (uint64_t) _shaderId;
}

id<MTLFunction>
HgiMetalShaderFunction::GetShaderId() const
{
    return _shaderId;
}

PXR_NAMESPACE_CLOSE_SCOPE
