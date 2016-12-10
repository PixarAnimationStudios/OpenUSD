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
/// \file wrapNamespaceEdit.cpp
#include "pxr/usd/sdf/namespaceEdit.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include <boost/python/class.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/init.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>

using namespace boost::python;

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
    const object& canEdit,
    const SdfNamespaceEdit& edit,
    std::string* whyNot)
{
    if (TfPyIsNone(canEdit)) {
        return true;
    }

    object result = TfPyCall<object>(canEdit)(edit);

    // We expect the result to be True or a tuple (False, str).  We'll
    // also accept for success a tuple (True, str) and we'll ignore the
    // string.  We'll also accept for failure just a str.
    {
        extract<tuple> e(result);
        if (e.check()) {
            tuple tupleResult = e();
            if (len(tupleResult) != 2) {
                TfPyThrowValueError("expected a 2-tuple");
            }
            str whyNotResult = extract<str>(tupleResult[1]);
            if (extract<bool>(tupleResult[0])) {
                return true;
            }
            else {
                if (whyNot) {
                    *whyNot = extract<std::string>(whyNotResult);
                }
                return false;
            }
        }
    }
    {
        extract<str> whyNotResult(result);
        if (whyNotResult.check()) {
            if (whyNot) {
                *whyNot = extract<std::string>(whyNotResult);
            }
            return false;
        }
    }
    if (not extract<bool>(result)) {
        // Need a string on failure.
        TfPyThrowValueError("expected a 2-tuple");
    }
    return true;
}

static
tuple
_Process(
    const SdfBatchNamespaceEdit& x,
    const object& hasObjectAtPath,
    const object& canEdit,
    bool fixBackpointers)
{
    // Return a pair (true,Edits) on success or (false,vector<string>) on
    // failure.
    SdfNamespaceEditVector edits;
    SdfNamespaceEditDetailVector details;
    bool result;
    if (TfPyIsNone(hasObjectAtPath)) {
        result = x.Process(&edits, SdfBatchNamespaceEdit::HasObjectAtPath(),
                           boost::bind(&_TranslateCanEdit, canEdit, _1, _2),
                           &details, fixBackpointers);
    }
    else {
        result = x.Process(&edits, TfPyCall<bool>(hasObjectAtPath),
                           boost::bind(&_TranslateCanEdit, canEdit, _1, _2),
                           &details, fixBackpointers);
    }
    if (result) {
        return make_tuple(object(true), object(edits));
    }
    else {
        return make_tuple(object(false), object(details));
    }
}

void
wrapNamespaceEditDetail()
{
    typedef SdfNamespaceEditDetail This;

    // Wrap SdfNamespaceEditDetail.
    scope s =
    class_<This>("NamespaceEditDetail", no_init)
        .def(init<>())
        .def(init<This::Result, const SdfNamespaceEdit&, const std::string&>())
        .def("__str__", &_StringifyEditDetail)
        .def("__repr__", &_ReprEditDetail)
        .def_readwrite("result", &This::result)
        .def_readwrite("edit", &This::edit)
        .def_readwrite("reason", &This::reason)
        .def(self == self)
        .def(self != self)
        ;

    // Wrap SdfNamespaceEditDetail::Result.
    TfPyWrapEnum<This::Result>();

    // Wrap SdfNamespaceEditDetailVector.
    to_python_converter<SdfNamespaceEditDetailVector,
                        TfPySequenceToPython<SdfNamespaceEditDetailVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfNamespaceEditDetailVector,
        TfPyContainerConversions::variable_capacity_policy>();
}

void
wrapBatchNamespaceEdit()
{
    typedef SdfBatchNamespaceEdit This;

    class_<This>("BatchNamespaceEdit", no_init)
        .def(init<>())
        .def(init<const This&>())
        .def(init<const SdfNamespaceEditVector&>())
        .def("__str__", &_StringifyBatchEdit)
        .def("__repr__", &_ReprBatchEdit)
        .def("Add", &_AddEdit)
        .def("Add", &_AddOldAndNew2)
        .def("Add", &_AddOldAndNew3)
        .add_property("edits",
            make_function(&This::GetEdits,
                          return_value_policy<return_by_value>()))
        .def("Process", &_Process,
            (arg("hasObjectAtPath"),
             arg("canEdit"),
             arg("fixBackpointers") = true))
        ;
}

static SdfNamespaceEdit::Index _atEnd = SdfNamespaceEdit::AtEnd;
static SdfNamespaceEdit::Index _same  = SdfNamespaceEdit::Same;

void
wrapNamespaceEdit()
{
    typedef SdfNamespaceEdit This;

    class_<This>("NamespaceEdit", no_init)
        .def(init<>())
        .def(init<const This::Path&, const This::Path&,
                  optional<This::Index> >())
        .def("__str__", &_StringifyEdit)
        .def("__repr__", &_ReprEdit)
        .def_readwrite("currentPath", &This::currentPath)
        .def_readwrite("newPath", &This::newPath)
        .def_readwrite("index", &This::index)
        .def_readonly("atEnd", &_atEnd)
        .def_readonly("same", &_same)
        .def(self == self)
        .def(self != self)

        .def("Remove", &This::Remove)
        .staticmethod("Remove")
        .def("Rename", &This::Rename)
        .staticmethod("Rename")
        .def("Reorder", &This::Reorder)
        .staticmethod("Reorder")
        .def("Reparent", &This::Reparent)
        .staticmethod("Reparent")
        .def("ReparentAndRename", &This::ReparentAndRename)
        .staticmethod("ReparentAndRename")
        ;

    // Wrap SdfNamespaceEditVector.
    to_python_converter<SdfNamespaceEditVector,
                        TfPySequenceToPython<SdfNamespaceEditVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfNamespaceEditVector,
        TfPyContainerConversions::variable_capacity_policy>();

    wrapNamespaceEditDetail();
    wrapBatchNamespaceEdit();
}
