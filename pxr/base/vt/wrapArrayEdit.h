//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_WRAP_ARRAY_EDIT_H
#define PXR_BASE_VT_WRAP_ARRAY_EDIT_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/arrayEditBuilder.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/wrapArray.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

template <class ArrayEdit>
void VtWrapArrayEdit()
{
    using namespace Vt_WrapArray;
    
    using Array = typename ArrayEdit::Array;
    using ElementType = typename ArrayEdit::ElementType;
    
    const std::string name = GetVtArrayName<Array>() + "Edit";
    nanobind::module_ m = TfPyGetCurrentNanobindModule();

    nanobind::class_<ArrayEdit>(m, name.c_str())
        .def(TfTypePythonClass())
        .def(nanobind::init<>())
        .def(nanobind::init<ArrayEdit const &>())
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)
        .def("__hash__",
             +[](ArrayEdit const &self) {
                 return TfHash{}(self);
             })
        .def("__str__", +[](ArrayEdit const &self) {
            return TfStringify(self);
        })
        .def("IsIdentity", &ArrayEdit::IsIdentity)
        .def("ComposeOver",
             +[](ArrayEdit const &self, ArrayEdit const &weaker) {
                 return self.ComposeOver(weaker);
             })
        .def("ComposeOver",
             +[](ArrayEdit const &self, Array const &weaker) {
                 return self.ComposeOver(weaker);
             })
        ;

    using Builder = VtArrayEditBuilder<ElementType>;
    nanobind::class_<Builder>(m, (name + "Builder").c_str())
        .def(nanobind::init<>())
        .def("Write", &Builder::Write,
             nanobind::arg("elem"), nanobind::arg("index"),
             nanobind::rv_policy::reference)
        .def("WriteRef", &Builder::WriteRef,
             nanobind::arg("srcIndex"), nanobind::arg("dstIndex"),
             nanobind::rv_policy::reference)
        .def("Insert", &Builder::Insert,
             nanobind::arg("elem"), nanobind::arg("index"),
             nanobind::rv_policy::reference)
        .def("InsertRef", &Builder::InsertRef,
             nanobind::arg("srcIndex"), nanobind::arg("dstIndex"),
             nanobind::rv_policy::reference)
        .def("Prepend", &Builder::Prepend,
             nanobind::arg("elem"), nanobind::rv_policy::reference)
        .def("PrependRef", &Builder::PrependRef,
             nanobind::arg("srcIndex"), nanobind::rv_policy::reference)
        .def("Append", &Builder::Append,
             nanobind::arg("elem"), nanobind::rv_policy::reference)
        .def("AppendRef", &Builder::AppendRef,
             nanobind::arg("srcIndex"), nanobind::rv_policy::reference)
        .def("EraseRef", &Builder::EraseRef,
             nanobind::arg("index"), nanobind::rv_policy::reference)
        .def("MinSize",
             +[](Builder &self, int64_t size) -> Builder & {
                 return self.MinSize(size);
             },
             nanobind::arg("size"), nanobind::rv_policy::reference)
        .def("MinSize",
             +[](Builder &self, int64_t size, ElementType const &fill) -> Builder & {
                 return self.MinSize(size, fill);
             },
             nanobind::arg("size"), nanobind::arg("fill"),
             nanobind::rv_policy::reference)
        .def("MaxSize", &Builder::MaxSize,
             nanobind::arg("size"), nanobind::rv_policy::reference)
        .def("SetSize",
             +[](Builder &self, int64_t size) -> Builder & {
                 return self.SetSize(size);
             },
             nanobind::arg("size"), nanobind::rv_policy::reference)
        .def("SetSize",
             +[](Builder &self, int64_t size, ElementType const &fill) -> Builder & {
                 return self.SetSize(size, fill);
             },
             nanobind::arg("size"), nanobind::arg("fill"),
             nanobind::rv_policy::reference)
        .def("FinalizeAndReset", &Builder::FinalizeAndReset)
        .def_static("Optimize", +[](ArrayEdit edit) {
            return Builder::Optimize(std::move(edit));
        }, nanobind::arg("edit"))
        ;
}

#define VT_WRAP_ARRAY_EDIT(unused, elem) \
    VtWrapArrayEdit<VtArrayEdit< VT_TYPE(elem) > >();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_WRAP_ARRAY_EDIT_H

