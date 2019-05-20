//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/utils.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/ndr/filesystemDiscoveryHelpers.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"
#include <MaterialXFormat/XmlIo.h>
#include <map>
#include <type_traits>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

using DocumentCache = std::map<std::string, mx::DocumentPtr>;

static
DocumentCache&
_GetCache()
{
    static DocumentCache cache;
    return cache;
}

// This exists in 1.35.5 and later as method copyContentFrom on Element.
static
void
_CopyContent(mx::ElementPtr dst, const mx::ConstElementPtr& source)
{
    dst->setSourceUri(source->getSourceUri());
    for (auto&& name: source->getAttributeNames()) {
        dst->setAttribute(name, source->getAttribute(name));
    }
    for (auto&& child: source->getChildren()) {
        _CopyContent(
            dst->addChildOfCategory(child->getCategory(), child->getName()),
            child);
    }
}

VtValue
_GetUsdValue(const std::string& valueString, const std::string& type)
{
    static const std::string filename("filename");
    static const std::string geomname("geomname");

#define CAST(Type, Cast) \
        if (value->isA<Type>()) { \
            return VtValue(static_cast<Cast>(value->asA<Type>())); \
        }
#define CASTV(Type, Cast) \
        if (value->isA<Type>()) { \
            auto&& vec = value->asA<Type>(); \
            Cast result; \
            for (size_t i = 0, n = vec.numElements(); i != n; ++i) { \
                result[i] = static_cast<Cast::ScalarType>(vec[i]); \
            } \
            return VtValue(result); \
        }
#define CASTM(Type, Cast) \
        if (value->isA<Type>()) { \
            auto&& mtx = value->asA<Type>(); \
            Cast result; \
            for (size_t j = 0, n = mtx.numRows(); j != n; ++j) { \
                for (size_t i = 0, m = mtx.numColumns(); i != m; ++i) { \
                    result.GetArray()[i + j * m] = \
                        static_cast<Cast::ScalarType>(mtx[j][i]); \
                } \
            } \
            return VtValue(result); \
        }
#define CASTA(Type, Cast) \
        if (value->isA<std::vector<Type>>()) { \
            auto&& vec = value->asA<std::vector<Type>>(); \
            VtArray<Cast> result; \
            result.reserve(vec.size()); \
            for (auto&& v: vec) { \
                result.push_back(static_cast<Cast>(v)); \
            } \
            return VtValue(result); \
        }

    if (valueString.empty()) {
        return VtValue();
    }

    // Get the value.
    if (auto value = mx::Value::createValueFromStrings(valueString, type)) {
        CAST(bool, bool)
        CAST(int, int)
        CAST(float, float)
        if (value->isA<std::string>()) {
            if (type == filename) {
                return VtValue(SdfAssetPath(value->asA<std::string>()));
            }
            if (type == geomname) {
                // XXX -- Check string is a valid path, maybe do some
                //        translations.  Also this result must be used
                //        as a relationship target;  SdfPath is not a
                //        valid value type.
                return VtValue(value->asA<std::string>());
            }
            return VtValue(value->asA<std::string>());
        }

        CASTA(bool, bool)
        CASTA(int, int)
        CASTA(float, float)
        CASTA(std::string, std::string)

        CASTV(mx::Color2, GfVec2f)
        CASTV(mx::Color3, GfVec3f)
        CASTV(mx::Color4, GfVec4f)
        CASTV(mx::Vector2, GfVec2f)
        CASTV(mx::Vector3, GfVec3f)
        CASTV(mx::Vector4, GfVec4f)

        CASTM(mx::Matrix33, GfMatrix3d)
        CASTM(mx::Matrix44, GfMatrix4d)

        // Aliases.
        CAST(long, int)
        CAST(double, float)

        TF_WARN("MaterialX unsupported type %s", type.c_str());
    }

    return VtValue();

#undef CAST
#undef CASTV
#undef CASTM
#undef CASTA
}

} // anonymous namespace

NdrStringVec
UsdMtlxGetSearchPathsFromEnvVar(const char* name)
{
    const std::string paths = TfGetenv(name);
    return !paths.empty() ? TfStringSplit(paths, ARCH_PATH_LIST_SEP) : NdrStringVec();
}

NdrStringVec
UsdMtlxMergeSearchPaths(const NdrStringVec& stronger,
                        const NdrStringVec& weaker)
{
    NdrStringVec result = stronger;
    result.insert(result.end(), weaker.begin(), weaker.end());
    return result;
}

NdrStringVec
UsdMtlxStandardLibraryPaths()
{
    static const auto materialxLibraryPaths =
        UsdMtlxMergeSearchPaths(
            UsdMtlxGetSearchPathsFromEnvVar("PXR_USDMTLX_STDLIB_SEARCH_PATHS"),
            NdrStringVec{
#ifdef PXR_MATERIALX_STDLIB_DIR
                PXR_MATERIALX_STDLIB_DIR
#endif
            }
        );
    return materialxLibraryPaths;
}

NdrStringVec
UsdMtlxStandardFileExtensions()
{
    static const auto extensions = NdrStringVec{ "mtlx" };
    return extensions;
}

MaterialX::ConstDocumentPtr 
UsdMtlxGetDocumentFromString(const std::string &mtlxXml)
{
    std::string hashStr = std::to_string(std::hash<std::string>{}(mtlxXml));
    // Look up in the cache, inserting a null document if missing.
    auto insertResult = _GetCache().emplace(hashStr, nullptr);
    auto& document = insertResult.first->second;
    if (insertResult.second) {       
        // cache miss
        try {
            auto doc = mx::createDocument();
            mx::readFromXmlString(doc, mtlxXml);
            document = doc;
        }
        catch (mx::Exception& x) {
            TF_DEBUG(NDR_PARSING).Msg("MaterialX error reading source XML: %s",
                                    x.what());
        }
    }

    return document;
}

MaterialX::ConstDocumentPtr
UsdMtlxGetDocument(const std::string& resolvedUri)
{
    // Look up in the cache, inserting a null document if missing.
    auto insertResult = _GetCache().emplace(resolvedUri, nullptr);
    auto& document = insertResult.first->second;
    if (!insertResult.second) {
        // Cache hit.
        return document;
    }

    // Read the file or the standard library files.
    if (resolvedUri.empty()) {
        document = mx::createDocument();
        for (auto&& fileResult:
                NdrFsHelpersDiscoverNodes(
                    UsdMtlxStandardLibraryPaths(),
                    UsdMtlxStandardFileExtensions(),
                    false)) {
            try {
                // Read the file.
                auto doc = mx::createDocument();
                mx::readFromXmlFile(doc, fileResult.resolvedUri);

                // Set the source URI on all (immediate) children of
                // the root so we can find the source later.  We
                // can't use the source URI on the document element
                // because we won't be copying that.
                for (auto&& element: doc->getChildren()) {
                    element->setSourceUri(fileResult.resolvedUri);
                }

                // Merge into library.
                _CopyContent(document, doc);
            }
            catch (mx::Exception& x) {
                TF_DEBUG(NDR_PARSING).Msg("MaterialX error reading '%s': %s",
                                          fileResult.resolvedUri.c_str(),
                                          x.what());
                continue;
            }
        }
    }
    else {
        try {
            auto doc = mx::createDocument();
            mx::readFromXmlFile(doc, resolvedUri);
            document = doc;
        }
        catch (mx::Exception& x) {
            TF_DEBUG(NDR_PARSING).Msg("MaterialX error reading '%s': %s",
                                      resolvedUri.c_str(),
                                      x.what());
        }
    }

    return document;
}

NdrVersion
UsdMtlxGetVersion(
    const MaterialX::ConstElementPtr& mtlx, bool* implicitDefault)
{
    TfErrorMark mark;

    // Use the default invalid version by default.
    auto version = NdrVersion().GetAsDefault();

    // Get the version, if any, otherwise use the invalid version.
    std::string versionString = mtlx->getAttribute("version");
    if (versionString.empty()) {
        // No version specified.  Use the default.
    }
    else {
        if (auto tmp = NdrVersion(versionString)) {
            version = tmp;
        }
        else {
            // Invalid version.  Use the default instead of failing.
        }
    }

    // Check for explicitly default/not default.
    if (implicitDefault) {
        std::string isdefault = mtlx->getAttribute("isdefaultversion");
        if (isdefault.empty()) {
            // No opinion means implicitly a (potential) default.
            *implicitDefault = true;
        }
        else {
            *implicitDefault = false;
            if (isdefault == "true") {
                // Explicitly the default.
                version = version.GetAsDefault();
            }
        }
    }

    mark.Clear();
    return version;
}

const std::string&
UsdMtlxGetSourceURI(const MaterialX::ConstElementPtr& element)
{
    for (auto scan = element; scan; scan = scan->getParent()) {
        const auto& uri = scan->getSourceUri();
        if (!uri.empty()) {
            return uri;
        }
    }
    return element->getSourceUri();
}

//
// MaterialX uses float for floating point values.  Sdr uses doubles
// so we convert float to double in UsdMtlxGetUsdType() and
// UsdMtlxGetUsdValue().
//

UsdMtlxUsdTypeInfo
UsdMtlxGetUsdType(const std::string& mtlxTypeName)
{
#define TUPLE3(sdf, exact, sdr) \
    UsdMtlxUsdTypeInfo(SdfValueTypeNames->sdf, exact, SdrPropertyTypes->sdr)
#define TUPLEX(sdf, exact, sdr) \
    UsdMtlxUsdTypeInfo(SdfValueTypeNames->sdf, exact, sdr)

    static const auto noMatch = TfToken();
    static const auto notFound =
        UsdMtlxUsdTypeInfo(SdfValueTypeName(), false, noMatch);

    static const auto table =
        std::unordered_map<std::string, UsdMtlxUsdTypeInfo>{
           { "boolean",       TUPLEX(Bool,          true,  noMatch) },
           { "color2array",   TUPLEX(Float2Array,   false, noMatch) },
           { "color2",        TUPLEX(Float2,        false, noMatch) },
           { "color3array",   TUPLE3(Color3fArray,  true,  Color)   },
           { "color3",        TUPLE3(Color3f,       true,  Color)   },
           { "color4array",   TUPLEX(Color4fArray,  true,  noMatch) },
           { "color4",        TUPLEX(Color4f,       true,  noMatch) },
           { "filename",      TUPLE3(Asset,         true,  String)  },
           { "floatarray",    TUPLE3(FloatArray,    true,  Float)   },
           { "float",         TUPLE3(Float,         true,  Float)   },
           { "geomnamearray", TUPLEX(StringArray,   false, noMatch) },
           { "geomname",      TUPLEX(String,        false, noMatch) },
           { "integerarray",  TUPLE3(IntArray,      true,  Int)     },
           { "integer",       TUPLE3(Int,           true,  Int)     },
           { "matrix33",      TUPLEX(Matrix3d,      true,  noMatch) },
           { "matrix44",      TUPLE3(Matrix4d,      true,  Matrix)  },
           { "stringarray",   TUPLE3(StringArray,   true,  String)  },
           { "string",        TUPLE3(String,        true,  String)  },
           { "vector2array",  TUPLEX(Float2Array,   false, noMatch) },
           { "vector2",       TUPLEX(Float2,        false, noMatch) },
           { "vector3array",  TUPLE3(Vector3fArray, true,  Vector)  },
           { "vector3",       TUPLE3(Vector3f,      true,  Vector)  },
           { "vector4array",  TUPLEX(Float4Array,   false, noMatch) },
           { "vector4",       TUPLEX(Float4,        false, noMatch) },
        };
#undef TUPLE3
#undef TUPLEX

    auto i = table.find(mtlxTypeName);
    return i == table.end() ? notFound : i->second;
}

VtValue
UsdMtlxGetUsdValue(
    const MaterialX::ConstElementPtr& mtlx,
    bool getDefaultValue)
{
    static const std::string defaultAttr("default");
    static const std::string typeAttr = mx::TypedElement::TYPE_ATTRIBUTE;
    static const std::string valueAttr = mx::ValueElement::VALUE_ATTRIBUTE;

    // Bail if no element.
    if (!mtlx) {
        return VtValue();
    }

    // Get the value string.
    auto&& valueString =
        getDefaultValue ? mtlx->getAttribute(defaultAttr)
                        : mtlx->getAttribute(valueAttr);

    // Get the value.
    return _GetUsdValue(valueString, mtlx->getAttribute(typeAttr));
}

std::vector<VtValue>
UsdMtlxGetPackedUsdValues(const std::string& values, const std::string& type)
{
    std::vector<VtValue> result;

    // It's impossible to parse packed arrays.  This is a MaterialX bug.
    if (TfStringEndsWith(type, "array")) {
        return result;
    }

    // Split on commas and convert each value separately.
    for (auto element: TfStringSplit(values, ",")) {
        auto typeErased = _GetUsdValue(TfStringTrim(element), type);
        if (typeErased.IsEmpty()) {
            result.clear();
            break;
        }
        result.push_back(typeErased);
    }
    return result;
}

std::vector<std::string>
UsdMtlxSplitStringArray(const std::string& s)
{
    return mx::splitString(s, mx::ARRAY_VALID_SEPARATORS);
}

PXR_NAMESPACE_CLOSE_SCOPE
