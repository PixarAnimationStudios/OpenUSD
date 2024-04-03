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

#include "pxr/base/arch/stackTrace.h"
#include "pxr/base/tf/callContext.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdPrmanDebugUtil {

static const int cw = 3; // length of "─"
static const char empty[] = "";
static const char line[] = "────────────────────";
static const char fmt[] = 
             "%01$+*20$.*17$f  %02$+*21$.*17$f  %03$+*22$.*17$f │ %04$+*23$.*17$f\n"
    "%19$*18$s%05$+*20$.*17$f  %06$+*21$.*17$f  %07$+*22$.*17$f │ %08$+*23$.*17$f\n"
    "%19$*18$s%09$+*20$.*17$f  %10$+*21$.*17$f  %11$+*22$.*17$f │ %12$+*23$.*17$f\n"
    "%19$*18$s%28$.*24$s"   "──%28$.*25$s"   "──%28$.*26$s"   "─┼─%28$.*27$s"   "\n"
    "%19$*18$s%13$+*20$.*17$f  %14$+*21$.*17$f  %15$+*22$.*17$f │ %16$+*23$.*17$f";

std::string
MatrixToString(const GfMatrix4d& mat, const int indent, const int precision)
{
    int width[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            width[i] = std::max(width[i],
                3 + precision + (int)log10(abs(mat[i][j])));
        }
    }

    return TfStringPrintf(fmt,
        mat[0][0], mat[1][0], mat[2][0], mat[3][0],
        mat[0][1], mat[1][1], mat[2][1], mat[3][1],
        mat[0][2], mat[1][2], mat[2][2], mat[3][2],
        mat[0][3], mat[1][3], mat[2][3], mat[3][3], 
        precision, indent, empty, 
        width[0], width[1], width[2], width[3],
        cw * width[0], cw * width[1], cw * width[2], cw * width[3],
        line);
}

std::string
MatrixToString(const RtMatrix4x4& mat, const int indent, const int precision)
{
    int width[4] = {0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            width[i] = std::max(width[i],
                3 + precision + (int)log10(abs(mat.m[i][j])));
        }
    }

    return TfStringPrintf(fmt,
        mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
        mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
        mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
        mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3], 
        precision, indent, empty, 
        width[0], width[1], width[2], width[3],
        cw * width[0], cw * width[1], cw * width[2], cw * width[3],
        line);
}

std::string
_GetParamPrefix(const RtParamList::ParamInfo& info)
{
    std::string out;
    switch (info.detail) {
        case RtDetailType::k_constant: out += "constant "; break;
        case RtDetailType::k_uniform: out += "uniform "; break;
        case RtDetailType::k_vertex: out += "vertex "; break;
        case RtDetailType::k_varying: out += "varying "; break;
        case RtDetailType::k_facevarying: out += "facevarying "; break;
        case RtDetailType::k_reference: out += "reference "; break;
        case RtDetailType::k_invalid: out += "invalid "; break;
    }
    switch (info.type) {
        case RtDataType::k_integer: out += "integer"; break;
        case RtDataType::k_float: out += "float"; break;
        case RtDataType::k_color: out += "color"; break;
        case RtDataType::k_point: out += "point"; break;
        case RtDataType::k_vector: out += "vector"; break;
        case RtDataType::k_normal: out += "normal"; break;
        case RtDataType::k_hpoint: out += "hpoint"; break;
        case RtDataType::k_mpoint: out += "mpoint"; break;
        case RtDataType::k_matrix: out += "matrix"; break;
        case RtDataType::k_string: out += "string"; break;
        case RtDataType::k_bxdf: out += "bxdf"; break;
        case RtDataType::k_lightfilter: out += "lightfilter"; break;
        case RtDataType::k_samplefilter: out += "samplefilter"; break;
        case RtDataType::k_displayfilter: out += "displayfilter"; break;
        case RtDataType::k_struct: out += "struct"; break;
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
    out += " = ";
    return out;
}

std::string 
_FormatParam(
    const RtParamList::ParamInfo& info,
    const RtParamList& _params,
    const int indent = 0) {

    static const char* Vec3Fmt = "(%f, %f, %f)";
    static const char* Vec4Fmt = "(%f, %f, %f, %f)";
    
    const std::string prefix = _GetParamPrefix(info);
    const int fullIndent = indent + (int)prefix.size();
    RtParamList& params = const_cast<RtParamList&>(_params);
    std::string val;
    switch (info.type) {
        case RtDataType::k_integer: {
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
            if (info.array) {
                const RtMatrix4x4* value = params.GetMpointArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += TfStringPrintf(",\n%*s", fullIndent, "");
                    }
                    RtMatrix4x4 mpoint = *(value+i);
                    val += MatrixToString(mpoint, fullIndent);
                }
            } else {
                RtMatrix4x4 value;
                if (params.GetMpoint(info.name, value)) {
                    val += MatrixToString(value, fullIndent);
                }
            }
            break;
        }
        case RtDataType::k_matrix: {
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetMatrixReferenceArray(
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
                if (params.GetMatrixReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtMatrix4x4* value = params.GetMatrixArray(
                    info.name, info.length);
                for (uint32_t i = 0; i < info.length; ++i) {
                    if (!val.empty()) {
                        val += TfStringPrintf(",\n%*s", fullIndent, "");
                    }
                    RtMatrix4x4 matrix = *(value+i);
                    val += MatrixToString(matrix, fullIndent);
                }
            } else {
                RtMatrix4x4 value;
                if (params.GetMatrix(info.name, value)) {
                    val += MatrixToString(value, fullIndent);
                }
            }
            break;
        }
        case RtDataType::k_string: {
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetStringReferenceArray(
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
                if (params.GetStringReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            } else if (info.array) {
                const RtUString* value = params.GetStringArray(
                    info.name, info.length);
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
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetBxdfReferenceArray(
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
                if (params.GetBxdfReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_lightfilter: {
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetLightFilterReferenceArray(
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
                if (params.GetLightFilterReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_samplefilter: {
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetSampleFilterReferenceArray(
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
                if (params.GetSampleFilterReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_displayfilter: {
            if (info.array && info.detail == RtDetailType::k_reference) {
                const RtUString* value = params.GetDisplayFilterReferenceArray(
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
                if (params.GetDisplayFilterReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
        case RtDataType::k_struct: 
        {
            if (info.detail == RtDetailType::k_reference) {
                RtUString value;
                if (params.GetStructReference(info.name, value)) {
                    val = TfStringPrintf("<%s>", value.CStr());
                }
            }
            break;
        }
    }
    return prefix + val;
}

std::string
RtParamListToString(const RtParamList& params, const int indent)
{
    unsigned numParams = params.GetNumParams();
    if (numParams == 0) {
        return "<empty param list>";
    }

    std::string out;
    RtParamList::ParamInfo info;
    for (unsigned pi = 0; pi < numParams; ++pi) {
        if (params.GetParamInfo(pi, info)) {
            if (!out.empty()) {
                out += "\n";
            }
            out += TfStringPrintf(
                "%*s%s", (pi == 0 ? 0 : indent), "", 
                _FormatParam(info, params, indent).c_str());
        }
    }
    return out;
}

std::string
GetCallerAsString(const TfCallContext& ctx)
{
    const std::string locator = TfStringPrintf("%s:%lu",
        ctx.GetFile(), ctx.GetLine());
    const std::vector<std::string>& lines = ArchGetStackTrace(10);
    size_t i = 0;
    while (i < 9 && lines[i].find(locator) == lines[i].npos) {
        i++;
    }
    if (i < 9) {
        const std::string& line = lines[i+1];
        return line.substr(28, line.find_first_of("(") - 28) + " at " + 
            line.substr(line.find_last_of("/") + 1);
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
