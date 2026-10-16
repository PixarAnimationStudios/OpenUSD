//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapNamespaceEdit.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/namespaceEdit.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <functional>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static
std::string
_StringifyEdit(const SdfNamespaceEdit& x)
{
    return TfStringify(x);
}

static
std::string
_ReprEdit(const SdfNamespaceEdit& x)
{
    if (x == SdfNamespaceEdit()) {
        return TfStringPrintf("%sNamespaceEdit()",
                              TF_PY_REPR_PREFIX.c_str());
    }
    else {
        return TfStringPrintf("%sNamespaceEdit(%s,%s,%d)",
                              TF_PY_REPR_PREFIX.c_str(),
                              TfPyRepr(x.currentPath).c_str(),
                              TfPyRepr(x.newPath).c_str(),
                              x.index);
    }
}

static
std::string
_StringifyEditDetail(const SdfNamespaceEditDetail& x)
{
    return TfStringify(x);
}

static
std::string
_ReprEditDetail(const SdfNamespaceEditDetail& x)
{
    if (x == SdfNamespaceEditDetail()) {
        return TfStringPrintf("%sNamespaceEditDetail()",
                              TF_PY_REPR_PREFIX.c_str());
    }
    else {
        return TfStringPrintf("%sNamespaceEditDetail(%s,%s,%s)",
                              TF_PY_REPR_PREFIX.c_str(),
                              TfPyRepr(x.result).c_str(),
                              TfPyRepr(x.edit).c_str(),
                              TfPyRepr(x.reason).c_str());
    }
}

static
std::string
_StringifyBatchEdit(const SdfBatchNamespaceEdit& x)
{
    std::vector<std::string> edits;
    for (const auto& edit : x.GetEdits()) {
        edits.push_back(_StringifyEdit(edit));
    }
    if (edits.empty()) {
        return TfStringPrintf("[]");
    }
    else {
        return TfStringPrintf("[%s]", TfStringJoin(edits, ",").c_str());
    }
}

static
std::string
_ReprBatchEdit(const SdfBatchNamespaceEdit& x)
{
    const SdfNamespaceEditVector& edits = x.GetEdits();
    if (edits.empty()) {
        return TfStringPrintf("%sBatchNamespaceEdit()",
                              TF_PY_REPR_PREFIX.c_str());
    }
    else {
        return TfStringPrintf("%sBatchNamespaceEdit(%s)",
                              TF_PY_REPR_PREFIX.c_str(),
                              TfPyRepr(edits).c_str());
    }
}

static
void
_AddEdit(SdfBatchNamespaceEdit& x, const SdfNamespaceEdit& edit)
{
    x.Add(edit);
}

static
void
_AddOldAndNew2(
    SdfBatchNamespaceEdit& x,
    const SdfNamespaceEdit::Path& currentPath,
    const SdfNamespaceEdit::Path& newPath)
{
    x.Add(currentPath, newPath);
}

static
void
_AddOldAndNew3(
    SdfBatchNamespaceEdit& x,
    const SdfNamespaceEdit::Path& currentPath,
    const SdfNamespaceEdit::Path& newPath,
    SdfNamespaceEdit::Index index)
{
    x.Add(currentPath, newPath, index);
}

static
bool
_TranslateCanEdit(
    const nanobind::object& canEdit,
    const SdfNamespaceEdit& edit,
    std::string* whyNot)
{
    if (canEdit.is_none()) {
        return true;
    }

    nanobind::object result = TfPyCall<nanobind::object>(canEdit)(edit);

    // We expect the result to be True or a tuple (False, str).  We'll
    // also accept for success a tuple (True, str) and we'll ignore the
    // string.  We'll also accept for failure just a str.
    if (nanobind::isinstance<nanobind::tuple>(result)) {
        nanobind::tuple tupleResult = nanobind::cast<nanobind::tuple>(result);
        if (nanobind::len(tupleResult) != 2) {
            TfPyThrowValueError("expected a 2-tuple");
        }
        std::string whyNotStr = nanobind::cast<std::string>(tupleResult[1]);
        if (nanobind::cast<bool>(tupleResult[0])) {
            return true;
        }
        else {
            if (whyNot) {
                *whyNot = whyNotStr;
            }
            return false;
        }
    }
    if (nanobind::isinstance<nanobind::str>(result)) {
        if (whyNot) {
            *whyNot = nanobind::cast<std::string>(result);
        }
        return false;
    }
    if (!nanobind::cast<bool>(result)) {
        // Need a string on failure.
        TfPyThrowValueError("expected a 2-tuple");
    }
    return true;
}

static
nanobind::tuple
_Process(
    const SdfBatchNamespaceEdit& x,
    const nanobind::object& hasObjectAtPath,
    const nanobind::object& canEdit,
    bool fixBackpointers)
{
    namespace ph = std::placeholders;
    // Return a pair (true,Edits) on success or (false,vector<string>) on
    // failure.
    SdfNamespaceEditVector edits;
    SdfNamespaceEditDetailVector details;
    bool result;
    if (hasObjectAtPath.is_none()) {
        result = x.Process(&edits, SdfBatchNamespaceEdit::HasObjectAtPath(),
                           std::bind(&_TranslateCanEdit, canEdit,
                                     ph::_1, ph::_2),
                           &details, fixBackpointers);
    }
    else {
        result = x.Process(&edits, TfPyCall<bool>(hasObjectAtPath),
                           std::bind(&_TranslateCanEdit, canEdit,
                                     ph::_1, ph::_2),
                           &details, fixBackpointers);
    }
    if (result) {
        return nanobind::make_tuple(true, nanobind::cast(edits));
    }
    else {
        return nanobind::make_tuple(false, nanobind::cast(details));
    }
}

void
wrapNamespaceEditDetail(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef SdfNamespaceEditDetail This;

    // Wrap SdfNamespaceEditDetail.
    nb::class_<This> s(m, "NamespaceEditDetail");
    s
        .def(nb::init<>())
        .def(nb::init<This::Result, const SdfNamespaceEdit&, const std::string&>())
        .def("__str__", &_StringifyEditDetail)
        .def("__repr__", &_ReprEditDetail)
        .def_rw("result", &This::result)
        .def_rw("edit", &This::edit)
        .def_rw("reason", &This::reason)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;

    // Wrap SdfNamespaceEditDetail::Result.
    TfNbWrapEnum<This::Result>(s);

    // Wrap SdfNamespaceEditDetailVector.
    TfPyContainerConversions::from_python_sequence<
        SdfNamespaceEditDetailVector,
        TfPyContainerConversions::variable_capacity_policy>();
}

void
wrapBatchNamespaceEdit(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef SdfBatchNamespaceEdit This;

    nb::class_<This>(m, "BatchNamespaceEdit")
        .def(nb::init<>())
        .def(nb::init<const This&>())
        .def(nb::init<const SdfNamespaceEditVector&>())
        .def("__str__", &_StringifyBatchEdit)
        .def("__repr__", &_ReprBatchEdit)
        .def("Add", &_AddEdit)
        .def("Add", &_AddOldAndNew2)
        .def("Add", &_AddOldAndNew3)
        .def_prop_ro("edits", &This::GetEdits, nb::rv_policy::copy)
        .def("Process",
            [](const SdfBatchNamespaceEdit& x,
               nb::handle hasObjectAtPath,
               nb::handle canEdit,
               bool fixBackpointers) {
                return _Process(
                    x,
                    nb::borrow<nb::object>(hasObjectAtPath),
                    nb::borrow<nb::object>(canEdit),
                    fixBackpointers);
            },
            nb::arg("hasObjectAtPath").none(),
            nb::arg("canEdit").none(),
            nb::arg("fixBackpointers") = true)
        ;
}

static SdfNamespaceEdit::Index _atEnd = SdfNamespaceEdit::AtEnd;
static SdfNamespaceEdit::Index _same  = SdfNamespaceEdit::Same;

} // anonymous namespace 

void
wrapNamespaceEdit(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef SdfNamespaceEdit This;

    nb::class_<This>(m, "NamespaceEdit")
        .def(nb::init<>())
        .def(nb::init<const This::Path&, const This::Path&, This::Index>(),
             nb::arg("currentPath"), nb::arg("newPath"),
             nb::arg("index") = SdfNamespaceEdit::AtEnd)
        .def("__str__", &_StringifyEdit)
        .def("__repr__", &_ReprEdit)
        .def_rw("currentPath", &This::currentPath)
        .def_rw("newPath", &This::newPath)
        .def_rw("index", &This::index)
        .def_ro_static("atEnd", &_atEnd)
        .def_ro_static("same", &_same)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def_static("Remove", &This::Remove)
        .def_static("Rename", &This::Rename)
        .def_static("Reorder", &This::Reorder)
        .def_static("Reparent", &This::Reparent)
        .def_static("ReparentAndRename", &This::ReparentAndRename)
        ;

    // Wrap SdfNamespaceEditVector.
    TfPyContainerConversions::from_python_sequence<
        SdfNamespaceEditVector,
        TfPyContainerConversions::variable_capacity_policy>();

    wrapNamespaceEditDetail(m);
    wrapBatchNamespaceEdit(m);
}
