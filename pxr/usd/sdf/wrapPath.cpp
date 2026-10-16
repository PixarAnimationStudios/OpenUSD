//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include <set>
#include <utility>
#include <vector>
#include <locale>

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/vt/valueFromPython.h"

#include <thread>
#include <atomic>

using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_Repr(const SdfPath &self) {
    if (self.IsEmpty()) {
        return TF_PY_REPR_PREFIX + "Path.emptyPath";
    }
    else {
        return string(TF_PY_REPR_PREFIX +
            "Path(")+TfPyRepr(self.GetAsString())+")";
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

static nanobind::object
_FindPrefixedRange(SdfPathVector const &paths, SdfPath const &prefix)
{
    pair<SdfPathVector::const_iterator, SdfPathVector::const_iterator>
        result = SdfPathFindPrefixedRange(paths.begin(), paths.end(), prefix);
    nanobind::object start = nanobind::cast(result.first - paths.begin());
    nanobind::object stop = nanobind::cast(result.second - paths.begin());
    return nanobind::steal<nanobind::object>(
        PySlice_New(start.ptr(), stop.ptr(), NULL));
}

static nanobind::object
_FindLongestPrefix(SdfPathVector const &paths, SdfPath const &path)
{
    SdfPathVector::const_iterator result =
        SdfPathFindLongestPrefix(paths.begin(), paths.end(), path);
    if (result == paths.end())
        return nanobind::none();
    return nanobind::cast(*result);
}

static nanobind::object
_FindLongestStrictPrefix(SdfPathVector const &paths, SdfPath const &path)
{
    SdfPathVector::const_iterator result =
        SdfPathFindLongestStrictPrefix(paths.begin(), paths.end(), path);
    if (result == paths.end())
        return nanobind::none();
    return nanobind::cast(*result);
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
_NonEmptyPath(SdfPath const &self)
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

struct Sdf_PyPathAncestorsRangeIterator {
    Sdf_PyPathAncestorsRangeIterator(
        const SdfPathAncestorsRange::iterator& begin,
        const SdfPathAncestorsRange::iterator& end)  
        : _it(begin), _end(end) {}

    SdfPath next() {
        _RaiseIfAtEnd();
        if (_didFirst) {
            ++_it;
            _RaiseIfAtEnd();
        }
        _didFirst = true;
        return *_it;
    }

private:
    void _RaiseIfAtEnd() const {
        if (_it == _end) {
            throw nanobind::stop_iteration("Iterator at end");
        }
    }

    SdfPathAncestorsRange::iterator _it, _end;
    bool _didFirst = false;
};

Sdf_PyPathAncestorsRangeIterator
Sdf_GetIterator(const SdfPathAncestorsRange& range)
{
    return {range.begin(), range.end()};
}

void Sdf_wrapAncestorsRange(nanobind::module_ &m)
{
    namespace nb = nanobind;
    using This = SdfPathAncestorsRange;

    nb::class_<This> s(m, "AncestorsRange");
    s
        .def(nb::init<const SdfPath&>())
        .def("GetPath", &This::GetPath, nb::rv_policy::copy)
        .def("__iter__", &Sdf_GetIterator)
        ;

    using Iter = Sdf_PyPathAncestorsRangeIterator;
    nb::class_<Iter>(s, "_iterator")
        .def("__next__", &Iter::next)
        ;
}

} // anonymous namespace 

void wrapPath(nanobind::module_ &m) {    
    namespace nb = nanobind;
    typedef SdfPath This;

    m.def("_PathGetDebuggerPathText", Sdf_PathGetDebuggerPathText);
    m.def("_PathStress", &_PathStress);
    m.def("_DumpPathStats", &Sdf_DumpPathStats);

    nb::class_<This> s( m, "Path" );
    s
        .def( nb::init< const string & >() )
        .def( nb::init<const SdfPath &>() )
        .def( nb::init<>() )

        // XXX: Document constants
        .def_ro_static("absoluteRootPath", &SdfPath::AbsoluteRootPath(),
            "The absolute path representing the top of the \n"
            "namespace hierarchy (</>).")
        .def_ro_static("reflexiveRelativePath", &SdfPath::ReflexiveRelativePath(), 
            "The relative path representing 'self' (<.>).")
        .def_ro_static("emptyPath", &SdfPath::EmptyPath(), 
            "The empty path.")

        .def_prop_ro("pathElementCount", &This::GetPathElementCount, 
            "The number of path elements in this path.")
        .def_prop_ro("pathString", &This::GetAsString,
            "The string representation of this path.")
        .def_prop_ro("name", &This::GetName, nb::rv_policy::copy,
            "The name of the prim, property or relational\n"
            "attribute identified by the path.\n\n"
            "'' for EmptyPath.  '.' for ReflexiveRelativePath.\n"
            "'..' for a path ending in ParentPathElement.\n")
        .def_prop_ro("elementString", &This::GetElementString, nb::rv_policy::copy,
            "The string representation of the terminal component of this path."
            "\nThis path can be reconstructed via \n"
            "thisPath.GetParentPath().AppendElementString(thisPath.element).\n"
            "None of absoluteRootPath, reflexiveRelativePath, nor emptyPath\n"
            "possess the above quality; their .elementString is the empty string.")
        .def_prop_ro("targetPath", &This::GetTargetPath, nb::rv_policy::copy,
            "The relational attribute target path for this path.\n\n"
            "EmptyPath if this is not a relational attribute path.")

        .def("GetAllTargetPathsRecursively", &_WrapGetAllTargetPathsRecursively)

        .def("GetVariantSelection", &This::GetVariantSelection)

        .def("IsAbsolutePath", &This::IsAbsolutePath)
        .def("IsAbsoluteRootPath", &This::IsAbsoluteRootPath)
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

        .def_prop_ro("isEmpty", &This::IsEmpty)
        
        .def("HasPrefix", &This::HasPrefix)

        .def("MakeAbsolutePath", &This::MakeAbsolutePath)
        .def("MakeRelativePath", &This::MakeRelativePath)

        .def("GetPrefixes",
             +[](SdfPath const &self, size_t n) { return self.GetPrefixes(n); },
             nb::arg("numPrefixes") = 0)
        
        .def("GetAncestorsRange", &This::GetAncestorsRange)

        .def("GetParentPath", &This::GetParentPath)
        .def("GetPrimPath", &This::GetPrimPath)
        .def("GetPrimOrPrimVariantSelectionPath",
             &This::GetPrimOrPrimVariantSelectionPath)
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
             nb::arg("oldPrefix"),
             nb::arg("newPrefix"),
             nb::arg("fixTargetPaths") = true)
        .def("GetCommonPrefix", &This::GetCommonPrefix)
        .def("RemoveCommonSuffix", &This::RemoveCommonSuffix,
             nb::arg("otherPath"),
             nb::arg("stopAtRootPrim") = false)
        .def("ReplaceName", &This::ReplaceName)
        .def("ReplaceTargetPath", &This::ReplaceTargetPath)
        .def("MakeAbsolutePath", &This::MakeAbsolutePath)
        .def("MakeRelativePath", &This::MakeRelativePath)

        .def_static("GetConciseRelativePaths", &This::GetConciseRelativePaths)

        .def_static("RemoveDescendentPaths", _RemoveDescendentPaths)
        .def_static("RemoveAncestorPaths", _RemoveAncestorPaths)

        .def_static("IsValidIdentifier", &This::IsValidIdentifier)

        .def_static("IsValidNamespacedIdentifier", &This::IsValidNamespacedIdentifier)

        .def_static("TokenizeIdentifier", &This::TokenizeIdentifier)
        .def_static("JoinIdentifier",
             (std::string (*)(const std::vector<std::string>&))
                 &This::JoinIdentifier)
        .def_static("JoinIdentifier",
             (std::string (*)(const std::string&, const std::string&))
                 &This::JoinIdentifier)

        .def_static("StripNamespace",
             (std::string (*)(const std::string&))
                 &This::StripNamespace)
        .def_static("StripPrefixNamespace", &This::StripPrefixNamespace)

        .def_static("IsValidPathString", &_IsValidPathString)

        .def_static("FindPrefixedRange", _FindPrefixedRange)

        .def_static("FindLongestPrefix", _FindLongestPrefix)

        .def_static("FindLongestStrictPrefix", _FindLongestStrictPrefix)

        .def("__str__", &This::GetAsString)

        .def("__bool__", _NonEmptyPath)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self > nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self >= nb::self)
        .def("__repr__", _Repr)
        .def("__hash__", &This::GetHash)
        ;

    s.attr("absoluteIndicator") = SdfPathTokens->absoluteIndicator.GetString(); 
    s.attr("childDelimiter") = SdfPathTokens->childDelimiter.GetString(); 
    s.attr("propertyDelimiter") = SdfPathTokens->propertyDelimiter.GetString(); 
    s.attr("relationshipTargetStart") = SdfPathTokens->relationshipTargetStart.GetString(); 
    s.attr("relationshipTargetEnd") = SdfPathTokens->relationshipTargetEnd.GetString(); 
    s.attr("parentPathElement") = SdfPathTokens->parentPathElement.GetString(); 
    s.attr("mapperIndicator") = SdfPathTokens->mapperIndicator.GetString(); 
    s.attr("expressionIndicator") = SdfPathTokens->expressionIndicator.GetString(); 
    s.attr("mapperArgDelimiter") = SdfPathTokens->mapperArgDelimiter.GetString(); 
    s.attr("namespaceDelimiter") = SdfPathTokens->namespaceDelimiter.GetString(); 

    // Register conversion for python list <-> vector<SdfPath>
    TfPyContainerConversions::from_python_sequence<
        SdfPathVector,
        TfPyContainerConversions::
            variable_capacity_all_items_convertible_policy >();

    // Register conversion for python list <-> set<SdfPath>
    TfPyContainerConversions::from_python_sequence<
        std::set<SdfPath>,
        TfPyContainerConversions::set_policy >();

    nb::implicitly_convertible<string, This>();

    VtValueFromPython<SdfPath>();

    Sdf_PathIsValidPathStringResult::
        WrapNb<Sdf_PathIsValidPathStringResult>(m, "_IsValidPathStringResult",
                                                "errorMessage");

    Sdf_wrapAncestorsRange(m);
}
