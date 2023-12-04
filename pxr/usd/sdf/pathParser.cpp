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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/pathParser.h"

PXR_NAMESPACE_OPEN_SCOPE

using namespace tao::TAO_PEGTL_NAMESPACE;

namespace Sdf_PathParser {

template <>
struct Action<ReflexiveRelative> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = SdfPath::ReflexiveRelativePath();
    }
};

template <>
struct Action<AbsoluteRoot> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = SdfPath::AbsoluteRootPath();
    }
};

template <>
struct Action<DotDot> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        if (pp.paths.back().IsEmpty()) {
            pp.paths.back() = SdfPath::ReflexiveRelativePath();
        }
        pp.paths.back() = pp.paths.back().GetParentPath();
    }
};

template <>
struct Action<PrimName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        if (pp.paths.back().IsEmpty()) {
            pp.paths.back() = SdfPath::ReflexiveRelativePath();
        }
        pp.paths.back() = pp.paths.back().AppendChild(GetToken(in));
    }
};

template <>
struct Action<VariantSetName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.varSetName = in.string();
    }
};

template <>
struct Action<VariantName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.varName = in.string();
    }
};

template <>
struct Action<VariantSelection> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() =
            pp.paths.back().AppendVariantSelection(pp.varSetName, pp.varName);
        pp.varSetName.clear();
        pp.varName.clear();
    }
};

template <>
struct Action<PropertyName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        if (pp.paths.back().IsEmpty()) {
            pp.paths.back() = SdfPath::ReflexiveRelativePath();
        }
        pp.paths.back() = pp.paths.back().AppendProperty(GetToken(in));
    }
};

template <>
struct Action<RelationalAttributeName> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() =
            pp.paths.back().AppendRelationalAttribute(GetToken(in));
    }
};

template <>
struct Action<TargetPathOpen> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.emplace_back();
    }
};

template <>
struct Action<TargetPath> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.targetType = PPContext::IsTargetPath;
    }
};

template <>
struct Action<MapperPath> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.targetType = PPContext::IsMapperPath;
    }
};

template <>
struct Action<MapperArg> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = pp.paths.back().AppendMapperArg(GetToken(in));
    }
};

template <>
struct Action<TargetPathClose> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        SdfPath targetPath = std::move(pp.paths.back());
        pp.paths.pop_back();
        if (pp.targetType == PPContext::IsTargetPath) {
            pp.paths.back() =
                pp.paths.back().AppendTarget(std::move(targetPath));
        }
        else {
            pp.paths.back() =
                pp.paths.back().AppendMapper(std::move(targetPath));
        }
    }
};

template <>
struct Action<Expression> {
    template <class Input>
    static void apply(Input const &in, PPContext &pp) {
        pp.paths.back() = pp.paths.back().AppendExpression();
    }
};

}

bool
Sdf_ParsePath(std::string const &pathStr, SdfPath *path, std::string *errMsg)
{
    Sdf_PathParser::PPContext context;
    try {
        if (!parse<must<Sdf_PathParser::Path, eolf>, Sdf_PathParser::Action>(
                string_input<> { pathStr, "" }, context)) {
            if (errMsg) {
                *errMsg = TfStringPrintf(
                    "Ill-formed SdfPath with no exception parsing <%s>",
                    pathStr.c_str());
            }
            if (path) {
                *path = SdfPath();
            }
            return false;
        }
    }
    catch (parse_error const &err) {
        if (errMsg) {
            *errMsg = TfStringPrintf(
                "Ill-formed SdfPath <%s>: %s", pathStr.c_str(), err.what());
        }
        if (path) {
            *path = SdfPath();
        }
        return false;
    }
    if (path) {
        *path = std::move(context.paths.back());
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
