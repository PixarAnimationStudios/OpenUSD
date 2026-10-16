//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>

using namespace std;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

////////////////////////////////////////////////////////////////////////
// Python -> C++ TfType conversion

// Provide conversion of either string typenames or Python class objects
// to TfTypes.
static TfType
_GetTfTypeFromPython(PyObject *p)
{
    if (nanobind::isinstance<TfType>(nanobind::handle(p)))
        return *nanobind::inst_ptr<TfType>(p);
    if (PyBytes_Check(p))
        return TfType::FindByName(
            std::string(PyBytes_AsString(p), PyBytes_Size(p)));
    if (PyUnicode_Check(p))
        return TfType::FindByName(nanobind::cast<string>(nanobind::handle(p)));
    else
        return TfType::FindByPythonClass(
            TfPyObjWrapper(nanobind::borrow<nanobind::object>(p)));
}

////////////////////////////////////////////////////////////////////////

static string
_Repr( const TfType & t )
{
    if (t.IsUnknown()) {
        return TF_PY_REPR_PREFIX + "Type.Unknown";
    } else {
        // Use TfPyRepr() to get Python-style escaping.
        return TF_PY_REPR_PREFIX +
            "Type.FindByName(" + TfPyRepr(t.GetTypeName()) + ")";
    }
}

static size_t
_TypeHash( const TfType & t )
{
    return TfHash()(t);
}

// These methods exist just to distinguish from equivalently named templates.
static bool
_IsA( const TfType & t1, const TfType & t2 )
{
    return t1.IsA(t2);
}

static TfType
_GetRoot()
{
    return TfType::GetRoot();
}

static TfType
_FindDerivedByName( TfType &t, const std::string & name )
{
    return t.FindDerivedByName(name);
}

static TfType
_FindByName( const std::string & name )
{
    return TfType::FindByName(name);
}

// Convert out parameter to return value
static std::vector<TfType>
_GetAllDerivedTypes( TfType & t )
{
    set<TfType> types;
    t.GetAllDerivedTypes(&types);
    return vector<TfType>( types.begin(), types.end() );
}

// Convert out parameter to return value
static std::vector<TfType>
_GetAllAncestorTypes( TfType & t )
{
    vector<TfType> types;
    t.GetAllAncestorTypes(&types);
    return types;
}

static TfType
_FindByPythonClass(const nanobind::object & classObj)
{
    // Guard against the potentially common mistake of calling Find() with a
    // string typename.  Rather than returning the unknown type (assuming
    // of course that we never declare Python's string type as a TfType),
    // we instead direct the caller to use FindByName().
    if (PyBytes_Check(classObj.ptr()) || PyUnicode_Check(classObj.ptr())) {
        TfPyThrowTypeError("String passed to Tf.Type.Find() -- you probably "
                           "want Tf.Type.FindByName() instead");
    }

    return TfType::FindByPythonClass(TfPyObjWrapper(classObj));
}

static void
_DumpTypeHierarchyRecursive( TfType t, int depth=0 )
{
    std::string indent;
    for (int i=0; i < depth; ++i)
        indent += "    ";

    printf("%s%s\n", indent.c_str(), t.GetTypeName().c_str());
    std::vector<TfType> derived = t.GetDirectlyDerivedTypes();
    TF_FOR_ALL(it, derived) {
        _DumpTypeHierarchyRecursive( *it, depth+1 );
    }
}

static void
_DumpTypeHierarchy( TfType t )
{
    _DumpTypeHierarchyRecursive(t);
}

} // anonymous namespace

static nanobind::tuple
_VecToTuple(std::vector<TfType> const& v) {
    PyObject* t = PyTuple_New(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        PyTuple_SET_ITEM(t, i, nanobind::cast(v[i]).release().ptr());
    }
    return nanobind::steal<nanobind::tuple>(t);
}

static nanobind::tuple
_StrVecToTuple(std::vector<std::string> const& v) {
    PyObject* t = PyTuple_New(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        PyTuple_SET_ITEM(t, i, nanobind::cast(v[i]).release().ptr());
    }
    return nanobind::steal<nanobind::tuple>(t);
}

void wrapType(nanobind::module_& m)
{
    typedef TfType This;
    namespace nb = nanobind;

    nb::class_<This> classDef(m, "Type");
    classDef
        .def(nb::init<>())
        .def(nb::init<const TfType &>())
        .def("__init__", [](This *self, nb::handle obj) {
            TfType t = _GetTfTypeFromPython(obj.ptr());
            if (t.IsUnknown()) {
                TfPyThrowTypeError(
                    TfStringPrintf("cannot convert %s to TfType; has that type "
                                   "been defined as a TfType?",
                                   TfPyRepr(obj).c_str()));
            }
            new (self) This(t);
        })

        .def("__bool__", [](const This &self) { return !(!self); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self > nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self >= nb::self)
        .def("__repr__", &_Repr)
        .def("__hash__", &_TypeHash)

        .def_static("GetRoot", &_GetRoot)

        .def_static("Find", [](nb::handle classObj) {
            return _FindByPythonClass(nb::borrow<nb::object>(classObj));
        })

        .def_static("FindByName", &_FindByName)
        .def_static("FindByName", [](nb::bytes name) {
            return _FindByName(std::string(name.c_str(), name.size()));
        })

        .def("FindDerivedByName", &_FindDerivedByName)
        .def("FindDerivedByName", [](TfType &t, nb::bytes name) {
            return _FindDerivedByName(
                t, std::string(name.c_str(), name.size()));
        })

        .def("IsA", &_IsA)

        .def_prop_ro("isUnknown", &This::IsUnknown)
        .def_prop_ro("isEnumType", &This::IsEnumType)
        .def_prop_ro("isPlainOldDataType", &This::IsPlainOldDataType)
        .def_prop_ro("sizeof", &This::GetSizeof)

        .def_prop_ro("typeName", &This::GetTypeName)

        .def_prop_ro("pythonClass", [](const This &self) {
            return nb::borrow<nb::object>(self.GetPythonClass().Get().ptr());
        })

        .def_prop_ro("baseTypes", [](const This &self) {
            return _VecToTuple(self.GetBaseTypes());
        })
        .def_prop_ro("derivedTypes", [](const This &self) {
            return _VecToTuple(self.GetDirectlyDerivedTypes());
        })

        .def("GetAliases", [](const This &self, TfType derivedType) {
            return _StrVecToTuple(self.GetAliases(derivedType));
        })
        .def("GetAllDerivedTypes", [](This &self) {
            return _VecToTuple(_GetAllDerivedTypes(self));
        })
        .def("GetAllAncestorTypes", [](This &self) {
            return _VecToTuple(_GetAllAncestorTypes(self));
        })

        .def_static("Define", [](nb::handle typeObj) {
            return TfType_DefinePythonTypeAndBases(
                nb::borrow<nb::object>(typeObj));
        })

        .def("AddAlias", [](const This &self, TfType base, const std::string &name) {
            self.AddAlias(base, name);
        })
        .def("AddAlias", [](const This &self, TfType base, nb::bytes name) {
            self.AddAlias(base, std::string(name.c_str(), name.size()));
        })

        .def_static("_DumpTypeHierarchy", &_DumpTypeHierarchy,
            "_DumpTypeHierarchy(TfType): "
            "Diagnostic method to print the type hierarchy beneath a given "
            "TfType.")
        ;

    nanobind::detail::implicitly_convertible(
        NB_CTX,
        &typeid(This),
        (void *)+[](PyTypeObject *, PyObject *,
                    nanobind::detail::cleanup_list *) noexcept -> bool {
            return true;
        },
        true);

    // Create an attribute to use for the repr() for the Unknown type.
    // We do not wrap the GetUnknownType() method, favoring this instead.
    classDef.attr("Unknown") = TfType();

    // Handle a sequence of TfTypes
    TfPyContainerConversions::from_python_sequence<
        std::vector< TfType >,
        TfPyContainerConversions::variable_capacity_policy >();

    TfPyContainerConversions::from_python_sequence<
        std::set<TfType> , 
        TfPyContainerConversions::set_policy >();
}
