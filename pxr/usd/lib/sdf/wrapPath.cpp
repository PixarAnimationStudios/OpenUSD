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

#include "pxr/pxr.h"

#include <set>
#include <utility>
#include <vector>
#include <locale>

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/vt/valueFromPython.h"

#include <boost/python.hpp>

#include <thread>
#include <atomic>

using namespace boost::python;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static vector<SdfPath> GetPrefixesHelper( const SdfPath &path ) {
    return path.GetPrefixes();
}

static string
_Repr(const SdfPath &self) {
    if (self == SdfPath::EmptyPath()) {
        return TF_PY_REPR_PREFIX + "Path.emptyPath";
    } else {
        return string(TF_PY_REPR_PREFIX +
            "Path(")+TfPyRepr(self.GetString())+")";
    }
}

static SdfPathVector
_RemoveDescendentPaths(SdfPathVector paths)
{
    SdfPath::RemoveDescendentPaths(&paths);
    return paths;
}

static SdfPathVector
_RemoveAncestorPaths(SdfPathVector paths)
{
    SdfPath::RemoveAncestorPaths(&paths);
    return paths;
}

static object
_FindPrefixedRange(SdfPathVector const &paths, SdfPath const &prefix)
{
    pair<SdfPathVector::const_iterator, SdfPathVector::const_iterator>
        result = SdfPathFindPrefixedRange(paths.begin(), paths.end(), prefix);
    object start(result.first - paths.begin());
    object stop(result.second - paths.begin());
    handle<> slice(PySlice_New(start.ptr(), stop.ptr(), NULL));
    return object(slice);
}

static object
_FindLongestPrefix(SdfPathVector const &paths, SdfPath const &path)
{
    SdfPathVector::const_iterator result =
        SdfPathFindLongestPrefix(paths.begin(), paths.end(), path);
    if (result == paths.end())
        return object();
    return object(*result);
}

static object
_FindLongestStrictPrefix(SdfPathVector const &paths, SdfPath const &path)
{
    SdfPathVector::const_iterator result =
        SdfPathFindLongestStrictPrefix(paths.begin(), paths.end(), path);
    if (result == paths.end())
        return object();
    return object(*result);
}

struct Sdf_PathIsValidPathStringResult : public TfPyAnnotatedBoolResult<string>
{
    Sdf_PathIsValidPathStringResult(bool val, string const &msg) :
        TfPyAnnotatedBoolResult<string>(val, msg) {}
};

static
Sdf_PathIsValidPathStringResult
_IsValidPathString(string const &pathString)
{
    string errMsg;
    bool valid = SdfPath::IsValidPathString(pathString, &errMsg);
    return Sdf_PathIsValidPathStringResult(valid, errMsg);
}

static
SdfPathVector
_WrapGetAllTargetPathsRecursively(SdfPath const self)
{
    SdfPathVector result;
    self.GetAllTargetPathsRecursively(&result);
    return result;
}

static bool
__nonzero__(SdfPath const &self)
{
    return !self.IsEmpty();
}

constexpr size_t NumStressPaths = 1 << 28;
constexpr size_t NumStressThreads = 16;
constexpr size_t StressIters = 3;
constexpr size_t MaxStressPathSize = 16;

static void _PathStressTask(size_t index, std::vector<SdfPath> &paths)
{
    auto pathsPerThread = NumStressPaths / NumStressThreads;
    auto begin = paths.begin() + pathsPerThread * index;
    auto end = begin + pathsPerThread;
    
    for (size_t stressIter = 0; stressIter != StressIters; ++stressIter) {
        for (auto i = begin; i != end; ++i) {
            SdfPath p = SdfPath::AbsoluteRootPath();
            //size_t offset = (i - begin) * index + stressIter;
            for (size_t j = 0; j != (rand() % MaxStressPathSize); ++j) {
                char name[2];
                name[0] = 'a' + (rand() % 26);
                name[1] = '\0';
                p = p.AppendChild(TfToken(name));
            }
            //if ((i-begin) % 1000 == 0) {
            //printf("%zu: storing path %zu: <%s>\n",
            //       (i-begin), index, p.GetText());
            //}
            *i = p;
        }
        printf("%zu did iter %zu\n", index, stressIter);
    }
}

static void _PathStress()
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    std::vector<SdfPath> manyPaths(NumStressPaths);
    std::vector<std::thread> threads(NumStressThreads);

    size_t index = 0;
    for (auto &t: threads) {
        t = std::thread(_PathStressTask, index++, std::ref(manyPaths));
    }
    for (auto &t: threads) {
        t.join();
    }
}

} // anonymous namespace 

void wrapPath() {    
    typedef SdfPath This;

    def("_PathStress", &_PathStress);
    def("_DumpPathStats", &Sdf_DumpPathStats);

    scope s = class_<This> ( "Path", init< const string & >() )
        .def( init<const SdfPath &>() )
        .def( init<>() )

        // XXX: Document constants
        .def_readonly("absoluteRootPath", &SdfPath::AbsoluteRootPath(),
            "The absolute path representing the top of the \n"
            "namespace hierarchy (</>).")
        .def_readonly("reflexiveRelativePath", &SdfPath::ReflexiveRelativePath(), 
            "The relative path representing 'self' (<.>).")
        .def_readonly("emptyPath", &SdfPath::EmptyPath(), 
            "The empty path.")

        .add_property("pathElementCount", &This::GetPathElementCount, 
            "The number of path elements in this path.")
        .add_property("pathString",
            make_function(&This::GetString, 
                return_value_policy<return_by_value>()),
            "The string representation of this path.")
        .add_property("name", make_function(&This::GetName, 
                    return_value_policy<copy_const_reference>()), 
            "The name of the prim, property or relational\n"
            "attribute identified by the path.\n\n"
            "'' for EmptyPath.  '.' for ReflexiveRelativePath.\n"
            "'..' for a path ending in ParentPathElement.\n")
        .add_property("elementString",
            make_function(&This::GetElementString, 
                return_value_policy<return_by_value>()),
            "The string representation of the terminal component of this path."
            "\nThis path can be reconstructed via \n"
            "thisPath.GetParentPath().AppendElementString(thisPath.element).\n"
            "None of absoluteRootPath, reflexiveRelativePath, nor emptyPath\n"
            "possess the above quality; their .elementString is the empty string.")
        .add_property("targetPath", make_function(&This::GetTargetPath, 
                    return_value_policy<copy_const_reference>()), 
            "The relational attribute target path for this path.\n\n"
            "EmptyPath if this is not a relational attribute path.")

        .def("GetAllTargetPathsRecursively", &_WrapGetAllTargetPathsRecursively,
             return_value_policy<TfPySequenceToList>())

        .def("GetVariantSelection", &This::GetVariantSelection,
             return_value_policy<TfPyPairToTuple>())

        .def("IsAbsolutePath", &This::IsAbsolutePath)
        .def("IsPrimPath", &This::IsPrimPath)
        .def("IsAbsoluteRootOrPrimPath", &This::IsAbsoluteRootOrPrimPath)
        .def("IsRootPrimPath", &This::IsRootPrimPath)
        .def("IsPropertyPath", &This::IsPropertyPath)
        .def("IsPrimPropertyPath", &This::IsPrimPropertyPath)
        .def("IsNamespacedPropertyPath", &This::IsNamespacedPropertyPath)
        .def("IsPrimVariantSelectionPath", &This::IsPrimVariantSelectionPath)
        .def("ContainsPrimVariantSelection", &This::ContainsPrimVariantSelection)
        .def("ContainsPropertyElements", &This::ContainsPropertyElements)
        .def("IsRelationalAttributePath", &This::IsRelationalAttributePath)
        .def("IsTargetPath", &This::IsTargetPath)
        .def("ContainsTargetPath", &This::ContainsTargetPath)
        .def("IsMapperPath", &This::IsMapperPath)
        .def("IsMapperArgPath", &This::IsMapperArgPath)
        .def("IsExpressionPath", &This::IsExpressionPath)

        .add_property("isEmpty", &This::IsEmpty)
        
        .def("HasPrefix", &This::HasPrefix)

        .def("MakeAbsolutePath", &This::MakeAbsolutePath)
        .def("MakeRelativePath", &This::MakeRelativePath)

        .def("GetPrefixes", GetPrefixesHelper,
              return_value_policy< TfPySequenceToList >(), 
            "Returns the prefix paths of this path.")

        .def("GetParentPath", &This::GetParentPath)
        .def("GetPrimPath", &This::GetPrimPath)
        .def("GetPrimOrPrimVariantSelectionPath", &This::GetPrimOrPrimVariantSelectionPath)
        .def("GetAbsoluteRootOrPrimPath", &This::GetAbsoluteRootOrPrimPath)
        .def("StripAllVariantSelections", &This::StripAllVariantSelections)

        .def("AppendPath", &This::AppendPath)
        .def("AppendChild", &This::AppendChild)
        .def("AppendProperty", &This::AppendProperty)
        .def("AppendVariantSelection", &This::AppendVariantSelection)
        .def("AppendTarget", &This::AppendTarget)
        .def("AppendRelationalAttribute", &This::AppendRelationalAttribute)
        .def("AppendMapper", &This::AppendMapper)
        .def("AppendMapperArg", &This::AppendMapperArg)
        .def("AppendExpression", &This::AppendExpression)
        .def("AppendElementString", &This::AppendElementString)

        .def("ReplacePrefix", &This::ReplacePrefix,
             ( arg("oldPrefix"),
               arg("newPrefix"),
               arg("fixTargetPaths") = true))
        .def("GetCommonPrefix", &This::GetCommonPrefix)
        .def("RemoveCommonSuffix", &This::RemoveCommonSuffix,
            arg("stopAtRootPrim") = false,
            return_value_policy< TfPyPairToTuple >())
        .def("ReplaceName", &This::ReplaceName)
        .def("ReplaceTargetPath", &This::ReplaceTargetPath)
        .def("MakeAbsolutePath", &This::MakeAbsolutePath)
        .def("MakeRelativePath", &This::MakeRelativePath)

        .def("GetConciseRelativePaths", &This::GetConciseRelativePaths,
            return_value_policy< TfPySequenceToList >())
            .staticmethod("GetConciseRelativePaths")

        .def("RemoveDescendentPaths", _RemoveDescendentPaths,
             return_value_policy< TfPySequenceToList >())
            .staticmethod("RemoveDescendentPaths")
        .def("RemoveAncestorPaths", _RemoveAncestorPaths,
             return_value_policy< TfPySequenceToList >())
            .staticmethod("RemoveAncestorPaths")

        .def("IsValidIdentifier", &This::IsValidIdentifier)
            .staticmethod("IsValidIdentifier")

        .def("IsValidNamespacedIdentifier", &This::IsValidNamespacedIdentifier)
            .staticmethod("IsValidNamespacedIdentifier")

        .def("TokenizeIdentifier",
             &This::TokenizeIdentifier)
            .staticmethod("TokenizeIdentifier")
        .def("JoinIdentifier",
             (std::string (*)(const std::vector<std::string>&))
                 &This::JoinIdentifier)
        .def("JoinIdentifier",
             (std::string (*)(const std::string&, const std::string&))
                 &This::JoinIdentifier)
            .staticmethod("JoinIdentifier")

        .def("StripNamespace",
             (std::string (*)(const std::string&))
                 &This::StripNamespace)
            .staticmethod("StripNamespace")
        .def("StripPrefixNamespace", &This::StripPrefixNamespace, 
                return_value_policy<TfPyPairToTuple>())
            .staticmethod("StripPrefixNamespace")

        .def("IsValidPathString", &_IsValidPathString)
             .staticmethod("IsValidPathString")

        .def("FindPrefixedRange", _FindPrefixedRange)
            .staticmethod("FindPrefixedRange")

        .def("FindLongestPrefix", _FindLongestPrefix)
            .staticmethod("FindLongestPrefix")

        .def("FindLongestStrictPrefix", _FindLongestStrictPrefix)
            .staticmethod("FindLongestStrictPrefix")

        .def("__str__", 
            make_function(&This::GetString, 
                return_value_policy<return_by_value>()))

        .def("__nonzero__", __nonzero__)

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self > self)
        .def(self <= self)
        .def(self >= self)
        .def("__repr__", _Repr)
        .def("__hash__", &This::GetHash)
        ;

    s.attr("menvaStart") = SdfPathTokens->menvaStart;
    s.attr("menvaEnd") = &SdfPathTokens->menvaEnd; 
    s.attr("absoluteIndicator") = &SdfPathTokens->absoluteIndicator; 
    s.attr("childDelimiter") = &SdfPathTokens->childDelimiter; 
    s.attr("propertyDelimiter") = &SdfPathTokens->propertyDelimiter; 
    s.attr("relationshipTargetStart") = &SdfPathTokens->relationshipTargetStart; 
    s.attr("relationshipTargetEnd") = &SdfPathTokens->relationshipTargetEnd; 
    s.attr("parentPathElement") = &SdfPathTokens->parentPathElement; 
    s.attr("mapperIndicator") = &SdfPathTokens->mapperIndicator; 
    s.attr("expressionIndicator") = &SdfPathTokens->expressionIndicator; 
    s.attr("mapperArgDelimiter") = &SdfPathTokens->mapperArgDelimiter; 
    s.attr("namespaceDelimiter") = &SdfPathTokens->namespaceDelimiter; 

    // Register conversion for python list <-> vector<SdfPath>
    to_python_converter<SdfPathVector, TfPySequenceToPython<SdfPathVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfPathVector,
        TfPyContainerConversions::
            variable_capacity_all_items_convertible_policy >();

    // Register conversion for python list <-> set<SdfPath>
    to_python_converter<SdfPathSet, TfPySequenceToPython<SdfPathSet> >();
    TfPyContainerConversions::from_python_sequence<
        std::set<SdfPath>,
        TfPyContainerConversions::set_policy >();

    implicitly_convertible<string, This>();

    VtValueFromPython<SdfPath>();

    Sdf_PathIsValidPathStringResult::
        Wrap<Sdf_PathIsValidPathStringResult>("_IsValidPathStringResult",
                                            "errorMessage");

}
