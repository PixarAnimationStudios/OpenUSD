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
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolvedPath.h"
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
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"
#include <MaterialXCore/Util.h>
#include <MaterialXFormat/XmlIo.h>
#include <map>
#include <type_traits>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMtlxTokens, USD_MTLX_TOKENS);

namespace {

using DocumentCache = std::map<std::string, mx::DocumentPtr>;

static
DocumentCache&
_GetCache()
{
    static DocumentCache cache;
    return cache;
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

// Return the contents of a search path environment variable
// as a vector of strings.  The path is split on the platform's
// native path list separator.
static const NdrStringVec
_GetSearchPathsFromEnvVar(const char* name)
{
    const std::string paths = TfGetenv(name);
    return !paths.empty() 
                ? TfStringSplit(paths, ARCH_PATH_LIST_SEP) 
                : NdrStringVec();
}

// Combines two search path lists.
static const NdrStringVec
_MergeSearchPaths(const NdrStringVec& stronger, const NdrStringVec& weaker)
{
    NdrStringVec result = stronger;
    result.insert(result.end(), weaker.begin(), weaker.end());
    return result;
}

static const NdrStringVec
_ComputeStdlibSearchPaths()
{
    // Get the MaterialX/libraries path(s)
    // This is used to indicate the location of the MaterialX/libraries folder 
    // if moved/changed from the path initialized in PXR_MATERIALX_STDLIB_DIR.
    NdrStringVec stdlibSearchPaths =
        _GetSearchPathsFromEnvVar("PXR_MTLX_STDLIB_SEARCH_PATHS");

    // Add path to the MaterialX standard library discovered at build time.
#ifdef PXR_MATERIALX_STDLIB_DIR
    stdlibSearchPaths =
        _MergeSearchPaths(stdlibSearchPaths, { PXR_MATERIALX_STDLIB_DIR });
#endif
    return stdlibSearchPaths;
}

const NdrStringVec&
UsdMtlxStandardLibraryPaths()
{
    static const auto materialxLibraryPaths = _ComputeStdlibSearchPaths();
    return materialxLibraryPaths;
}

const NdrStringVec&
UsdMtlxCustomSearchPaths()
{
    // Get the location of any additional custom mtlx files outside 
    // of the standard library files.
    static const auto materialxCustomSearchPaths =
        _GetSearchPathsFromEnvVar("PXR_MTLX_PLUGIN_SEARCH_PATHS");
    return materialxCustomSearchPaths;
}

const NdrStringVec&
UsdMtlxSearchPaths()
{
    static const auto materialxSearchPaths = 
        _MergeSearchPaths(
            UsdMtlxCustomSearchPaths(), UsdMtlxStandardLibraryPaths());
    return materialxSearchPaths;
}

NdrStringVec
UsdMtlxStandardFileExtensions()
{
    static const auto extensions = NdrStringVec{ "mtlx" };
    return extensions;
}

#if AR_VERSION > 1
static void
_ReadFromAsset(mx::DocumentPtr doc, const ArResolvedPath& resolvedPath,
               const mx::FileSearchPath& searchPath = mx::FileSearchPath(),
               const mx::XmlReadOptions* readOptionsIn = nullptr)
{
    std::shared_ptr<const char> buffer;
    size_t bufferSize = 0;

    std::tie(buffer, bufferSize) = [&resolvedPath]() {
        const std::shared_ptr<ArAsset> asset = 
            ArGetResolver().OpenAsset(resolvedPath);
        return asset ?
            std::make_pair(asset->GetBuffer(), asset->GetSize()) :
            std::make_pair(std::shared_ptr<const char>(), 0ul);
    }();

    if (!buffer) {
        TF_RUNTIME_ERROR("Unable to open MaterialX document '%s'",
                         resolvedPath.GetPathString().c_str());
        return;
    }

    // Copy contents of file into a string to pass to MaterialX. 
    // MaterialX does have a std::istream-based API so we could try to use that
    // if the string copy becomes a burden.
    const std::string s(buffer.get(), bufferSize);

    // Set up an XmlReadOptions with a callback to this function so that we
    // can also handle any XInclude paths using the ArAsset API.
    mx::XmlReadOptions readOptions =
        readOptionsIn ? *readOptionsIn : mx::XmlReadOptions();
    readOptions.readXIncludeFunction = 
        [&resolvedPath](mx::DocumentPtr newDoc, 
                        const mx::FilePath& newFilename,
                        const mx::FileSearchPath& newSearchPath, 
                        const mx::XmlReadOptions* newReadOptions)
        {
            // MaterialX does not anchor XInclude'd file paths to the source
            // document's path, so we need to do that ourselves to pass to Ar.
            std::string newFilePath;

            if (ArIsPackageRelativePath(resolvedPath)) {
                // If the source file is a package like foo.usdz[a/b/doc.mx],
                // we want to anchor the new filename to the packaged path, so
                // we'd wind up with foo.usdz[a/b/included.mx].
                std::string packagePath, packagedPath;
                std::tie(packagePath, packagedPath) = 
                    ArSplitPackageRelativePathInner(resolvedPath);

                std::string newPackagedPath = TfGetPathName(packagedPath);
                newPackagedPath = TfNormPath(newPackagedPath.empty() ? 
                    newFilename.asString() : 
                    TfStringCatPaths(newPackagedPath, newFilename));

                newFilePath = ArJoinPackageRelativePath(
                    packagePath, newPackagedPath);
            }
            else {
                // Otherwise use ArResolver to anchor newFilename to the
                // source file.
                newFilePath = ArGetResolver().CreateIdentifier(
                    newFilename, resolvedPath);
            }

            const ArResolvedPath newResolvedPath = ArGetResolver().Resolve(
                newFilePath);
            if (!newResolvedPath) {
                TF_RUNTIME_ERROR("Unable to open MaterialX document '%s'",
                                 newFilePath.c_str());
                return;
            }

            _ReadFromAsset(newDoc, newResolvedPath, newSearchPath,
                           newReadOptions);
        };

    mx::readFromXmlString(doc, s, searchPath, &readOptions);
}
#endif

mx::DocumentPtr
UsdMtlxReadDocument(const std::string& resolvedPath)
{
    try {
        mx::DocumentPtr doc = mx::createDocument();

#if AR_VERSION == 1
        mx::readFromXmlFile(doc, resolvedPath);
        return doc;
#else
        // If resolvedPath points to a file on disk read from it directly
        // otherwise use the more general ArAsset API to read it from
        // whatever backing store it points to.
        if (TfIsFile(resolvedPath)) {
            mx::readFromXmlFile(doc, resolvedPath);
            return doc;
        }
        else {
            TfErrorMark m;
            _ReadFromAsset(doc, ArResolvedPath(resolvedPath));
            if (m.IsClean()) {
                return doc;
            }
        }
#endif
    }
    catch (mx::ExceptionFoundCycle& x) {
        TF_RUNTIME_ERROR("MaterialX cycle found reading '%s': %s", 
                         resolvedPath.c_str(), x.what());
        return nullptr;
    }
    catch (mx::Exception& x) {
        TF_RUNTIME_ERROR("MaterialX error reading '%s': %s",
                         resolvedPath.c_str(), x.what());
        return nullptr;
    }

    return nullptr;
}

mx::ConstDocumentPtr 
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

mx::ConstDocumentPtr
UsdMtlxGetDocument(const std::string& resolvedUri)
{
    // Look up in the cache, inserting a null document if missing.
    auto insertResult = _GetCache().emplace(resolvedUri, nullptr);
    auto& document = insertResult.first->second;
    if (!insertResult.second) {
        // Cache hit.
        return document;
    }

    TfErrorMark m;

    // Read the file or the standard library files.
    if (resolvedUri.empty()) {
        document = mx::createDocument();
        for (auto&& fileResult:
                NdrFsHelpersDiscoverFiles(
                    UsdMtlxStandardLibraryPaths(),
                    UsdMtlxStandardFileExtensions(),
                    false)) {

            // Read the file. If this fails due to an exception, a runtime
            // error will be raised so we can just skip to the next file.
            auto doc = UsdMtlxReadDocument(fileResult.resolvedUri);
            if (!doc) {
                continue;
            }

            try {
                
                // Merge this document into the global library
                // This properly sets the attributes on the destination 
                // elements, like source URI and namespace
                document->importLibrary(doc);
            }
            catch (mx::Exception& x) {
                TF_RUNTIME_ERROR("MaterialX error reading '%s': %s",
                                 fileResult.resolvedUri.c_str(),
                                 x.what());
            }
        }
    }
    else {
        document = UsdMtlxReadDocument(resolvedUri);
    }

    if (!m.IsClean()) {
        for (const auto& error : m) {
            TF_DEBUG(NDR_PARSING).Msg("%s\n", error.GetCommentary().c_str());
        }
        m.Clear();
    }

    return document;
}

NdrVersion
UsdMtlxGetVersion(
    const mx::ConstInterfaceElementPtr& mtlx, bool* implicitDefault)
{
    TfErrorMark mark;

    // Use the default invalid version by default.
    auto version = NdrVersion().GetAsDefault();

    // Get the version, if any, otherwise use the invalid version.
    std::string versionString = mtlx->getVersionString();
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
        const bool isdefault = mtlx->getDefaultVersion();
        if (isdefault) {
            *implicitDefault = false;
            version = version.GetAsDefault();
        }
        else {
            // No opinion means implicitly a (potential) default.
            *implicitDefault = true;
        }
    }

    mark.Clear();
    return version;
}

const std::string&
UsdMtlxGetSourceURI(const mx::ConstElementPtr& element)
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
#define TUPLEN(sdf, exact, sdr, sz) \
    UsdMtlxUsdTypeInfo(SdfValueTypeNames->sdf, exact, SdrPropertyTypes->sdr, sz)
#define TUPLEX(sdf, exact, sdr) \
    UsdMtlxUsdTypeInfo(SdfValueTypeNames->sdf, exact, sdr)

    static const auto noMatch = TfToken();
    static const auto notFound =
        UsdMtlxUsdTypeInfo(SdfValueTypeName(), false, noMatch);

    static const auto table =
        std::unordered_map<std::string, UsdMtlxUsdTypeInfo>{
           { "boolean",       TUPLEX(Bool,          true,  noMatch) },
           { "color2array",   TUPLEX(Float2Array,   false, noMatch) },
           { "color2",        TUPLEN(Float2,        false, Float, 2)},
           { "color3array",   TUPLE3(Color3fArray,  true,  Color)   },
           { "color3",        TUPLE3(Color3f,       true,  Color)   },
           { "color4array",   TUPLEX(Color4fArray,  true,  noMatch) },
           { "color4",        TUPLEN(Color4f,       true,  Float, 4)},
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
           { "vector2array",  TUPLEX(Float2Array,   true,  noMatch) },
           { "vector2",       TUPLEN(Float2,        true,  Float, 2)},
           { "vector3array",  TUPLEX(Float3Array,   true,  noMatch) },
           { "vector3",       TUPLEN(Float3,        true,  Float, 3)},
           { "vector4array",  TUPLEX(Float4Array,   true,  noMatch) },
           { "vector4",       TUPLEN(Float4,        true,  Float, 4)},
        };
#undef TUPLE3
#undef TUPLEX

    auto i = table.find(mtlxTypeName);
    return i == table.end() ? notFound : i->second;
}

VtValue
UsdMtlxGetUsdValue(
    const mx::ConstElementPtr& mtlx,
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
