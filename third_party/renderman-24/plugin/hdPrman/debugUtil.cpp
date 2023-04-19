//
// Copyright 2023 Pixar
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
#include "hdPrman/debugUtil.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdPrmanDebugUtil {

std::string 
_FormatParam(const RtParamList::ParamInfo& info, const RtParamList& _params) {
    static const char* Vec3Fmt = "(%f, %f, %f)";
    static const char* Vec4Fmt = "(%f, %f, %f, %f)";
    static const char* Mat4x4Fmt = 
        "((%f, %f, %f, %f), (%f, %f, %f, %f), (%f, %f, %f, %f), (%f, %f, %f, %f))";
    
    std::string out;
    std::string val;
    switch (info.detail) {
        case RtDetailType::k_constant: out += "constant "; break;
        case RtDetailType::k_uniform: out += "uniform "; break;
        case RtDetailType::k_vertex: out += "vertex "; break;
        case RtDetailType::k_varying: out += "varying "; break;
        case RtDetailType::k_facevarying: out += "facevarying "; break;
        case RtDetailType::k_reference: out += "reference "; break;
        case RtDetailType::k_invalid: out += "invalid "; break;
    }
    RtParamList& params = const_cast<RtParamList&>(_params);
    switch (info.type) {
        case RtDataType::k_integer: {
            out += "integer";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetIntegerReferenceArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetIntegerReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const int32_t* value = params.GetIntegerArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    val += TfStringPrintf("%i", *(value+i));
                }
                val += ")";
            } else {
                int32_t value;
                if (params.GetInteger(info.name, value)) {
                    val = TfStringPrintf("%i", value);
                }
            }
            break;
        }
        case RtDataType::k_float: {
            out += "float";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetFloatReferenceArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetFloatReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const float* value = params.GetFloatArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    val += TfStringPrintf("%f", *(value+i));
                }
                val += ")";
            } else {
                float value;
                if (params.GetFloat(info.name, value)) {
                    val = TfStringPrintf("%f", value);
                }
            }
            break;
        }
        case RtDataType::k_color: {
            out += "color";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetColorReferenceArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetColorReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtColorRGB* value = params.GetColorArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    RtColorRGB color = *(value+i);
                    val += TfStringPrintf(Vec3Fmt, color.r, color.g, color.b);
                }
                val += ")";
            } else {
                RtColorRGB value;
                if (params.GetColor(info.name, value)) {
                    val = TfStringPrintf(Vec3Fmt, value.r, value.g, value.b);
                }
            }
            break;
        }
        case RtDataType::k_point: {
            out += "point";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetPointReferenceArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetPointReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtPoint3* value = params.GetPointArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    RtPoint3 point = *(value+i);
                    val += TfStringPrintf(Vec3Fmt, point.x, point.y, point.z);
                }
                val += ")";
            } else {
                RtPoint3 value;
                if (params.GetPoint(info.name, value)) {
                    val = TfStringPrintf(Vec3Fmt, value.x, value.y, value.z);
                }
            }
            break;
        }
        case RtDataType::k_vector: {
            out += "vector";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetVectorReferenceArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetVectorReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtVector3* value = params.GetVectorArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    RtPoint3 vector = *(value+i);
                    val += TfStringPrintf(Vec3Fmt, vector.x, vector.y, vector.z);
                }
                val += ")";
            } else {
                RtVector3 value;
                if (params.GetVector(info.name, value)) {
                    val = TfStringPrintf(Vec3Fmt, value.x, value.y, value.z);
                }
            }
            break;
        }
        case RtDataType::k_normal: {
            out += "normal";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetNormalReferenceArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetNormalReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtNormal3* value = params.GetNormalArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    RtNormal3 normal = *(value+i);
                    val += TfStringPrintf(Vec3Fmt, normal.x, normal.y, normal.z);
                }
                val += ")";
            } else {
                RtNormal3 value;
                if (params.GetNormal(info.name, value)) {
                    val = TfStringPrintf(Vec3Fmt, value.x, value.y, value.z);
                }
            }
            break;
        }
        case RtDataType::k_hpoint: {
            out += "hpoint";
            if (info.array) {
                const RtPoint4* value = params.GetHpointArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    RtPoint4 point = *(value+i);
                    val += TfStringPrintf(
                        Vec4Fmt, point.x, point.y, point.z, point.w);
                }
                val += ")";
            } else {
                RtPoint4 value;
                if (params.GetHpoint(info.name, value)) {
                    val = TfStringPrintf(
                        Vec4Fmt, value.x, value.y, value.z, value.w);
                }
            }
            break;
        }
        case RtDataType::k_mpoint: {
            out += "mpoint";
            if (info.array) {
                const RtMatrix4x4* value = params.GetMpointArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    RtMatrix4x4 mpoint = *(value+i);
                    val += TfStringPrintf(
                        Mat4x4Fmt,
                        mpoint[0], mpoint[1], mpoint[2], mpoint[3],
                        mpoint[4], mpoint[5], mpoint[6], mpoint[7],
                        mpoint[8], mpoint[9], mpoint[10], mpoint[11],
                        mpoint[12], mpoint[13], mpoint[14], mpoint[15]);
                }
                val += ")";
            } else {
                RtMatrix4x4 value;
                if (params.GetMpoint(info.name, value)) {
                    val = TfStringPrintf(
                        Mat4x4Fmt,
                        value[0], value[1], value[2], value[3],
                        value[4], value[5], value[6], value[7],
                        value[8], value[9], value[10], value[11],
                        value[12], value[13], value[14], value[15]);
                }
            }
            break;
        }
        case RtDataType::k_matrix: {
            out += "matrix";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetMatrixReferenceArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetMatrixReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtMatrix4x4* value = params.GetMatrixArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    RtMatrix4x4 matrix = *(value+i);
                    val += TfStringPrintf(
                        Mat4x4Fmt,
                        matrix[0], matrix[1], matrix[2], matrix[3],
                        matrix[4], matrix[5], matrix[6], matrix[7],
                        matrix[8], matrix[9], matrix[10], matrix[11],
                        matrix[12], matrix[13], matrix[14], matrix[15]);
                }
                val += ")";
            } else {
                RtMatrix4x4 value;
                if (params.GetMatrix(info.name, value)) {
                    val = TfStringPrintf(
                        Mat4x4Fmt,
                        value[0], value[1], value[2], value[3],
                        value[4], value[5], value[6], value[7],
                        value[8], value[9], value[10], value[11],
                        value[12], value[13], value[14], value[15]);
                }
            }
            break;
        }
        case RtDataType::k_string: {
            out += "string";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetStringReferenceArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetStringReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtUString* value = params.GetStringArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "(";
                    }
                    val += TfStringPrintf("%s", (*(value+i)).CStr());
                }
                val += ")";
            } else {
                RtUString value;
                if (params.GetString(info.name, value)) {
                    val = TfStringPrintf("%s", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_bxdf: {
            out += "bxdf";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetBxdfReferenceArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetBxdfReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_lightfilter: {
            out += "lightfilter";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetLightFilterReferenceArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetLightFilterReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_samplefilter: {
            out += "samplefilter";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetSampleFilterReferenceArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetSampleFilterReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_displayfilter: {
            out += "displayfilter";
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetDisplayFilterReferenceArray(info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += ", ";
                    } else {
                        val += "[";
                    }
                    val += TfStringPrintf("<%s>", (*(value+i)).CStr());
                }
                val += "]";
            } else if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetDisplayFilterReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_struct: 
        {
            out += "struct";
            if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetStructReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
    }
    if (info.array) {
        out += TfStringPrintf("[%u]", info.length);
    }
    out += TfStringPrintf(" %s", info.name.CStr());
    if (info.detail == RtDetailType::k_reference) {
        out += ".connect";
    }
    if (info.motion) {
        out += ".timesamples";
    }
    out += TfStringPrintf(" = %s", val.c_str());
    return out;
}

std::string
RtParamListToString(const RtParamList& params, const std::string& name)
{
    std::string out = TfStringPrintf("****** RtParamList: %s ******\n", name.c_str());
    unsigned numParams = params.GetNumParams();
    RtParamList::ParamInfo info;
    for (unsigned pi = 0; pi < numParams; ++pi) {
        if (params.GetParamInfo(pi, info)) {
            out += TfStringPrintf("    %s\n", _FormatParam(info, params).c_str());
        }
    }
    return out + std::string(27 + name.size(), '*') + "\n";
}

std::string
GetCallerAsString(const TfCallContext& ctx)
{
    const std::string locator = TfStringPrintf("%s:%lu", ctx.GetFile(), ctx.GetLine());
    const std::vector<std::string>& lines = ArchGetStackTrace(10);
    size_t i = 0;
    while (i < 9 && lines[i].find(locator) == lines[i].npos) {
        i++;
    }
    if (i < 9) {
        const std::string& line = lines[i+1];
        return line.substr(28, line.find_first_of("(") - 28) + " at " + line.substr(line.find_last_of("/") + 1);
    }
    return "*** couldn't find caller ***";
}

std::string
SdfPathVecToString(const std::vector<SdfPath>& vec)
{
    std::string out;
    for (const SdfPath& path : vec) {
        if (!out.empty()) {
            out += ", ";
        }
        out += TfStringPrintf("<%s>", path.GetText());
    }
    return out;
}

}
PXR_NAMESPACE_CLOSE_SCOPE
