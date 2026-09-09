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

#include "pxr/external/boost/python/class.hpp"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

template <class ArrayEdit>
void VtWrapArrayEdit()
{
    using namespace pxr_boost::python;
    using namespace Vt_WrapArray;
    
    using Array = typename ArrayEdit::Array;
    using ElementType = typename ArrayEdit::ElementType;
    
    const std::string name = GetVtArrayName<Array>() + "Edit";

    class_<ArrayEdit>(name.c_str())
        .def(init<ArrayEdit const &>())
        .def(self == self)
        .def(self != self)
        .def("__hash__",
             +[](ArrayEdit const &self) {
                 return TfHash{}(self);
             })
        .def(self_ns::str(self_ns::self))
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
    class_<Builder>((name + "Builder").c_str())
        .def("Write", &Builder::Write,
             (arg("elem"), arg("index")), return_self<>())
        .def("WriteRef", &Builder::WriteRef,
             (arg("srcIndex"), arg("dstIndex")), return_self<>())
        .def("Insert", &Builder::Insert,
             (arg("elem"), arg("index")), return_self<>())
        .def("InsertRef", &Builder::InsertRef,
             (arg("srcIndex"), arg("dstIndex")), return_self<>())
        .def("Prepend", &Builder::Prepend,
             (arg("elem")), return_self<>())
        .def("PrependRef", &Builder::PrependRef,
             (arg("srcIndex")), return_self<>())
        .def("Append", &Builder::Append, (arg("elem")), return_self<>())
        .def("AppendRef", &Builder::AppendRef,
             (arg("srcIndex")), return_self<>())
        .def("EraseRef", &Builder::EraseRef, (arg("index")), return_self<>())
        .def("MinSize",
             +[](Builder &self, int64_t size) {
                 self.MinSize(size);
             },
             (arg("size")), return_self<>())
        .def("MinSize",
             +[](Builder &self, int64_t size, ElementType const &fill) {
                 self.MinSize(size, fill);
             }, (arg("size"), arg("fill")), return_self<>())
        .def("MaxSize", &Builder::MaxSize, (arg("size")), return_self<>())
        .def("SetSize",
             +[](Builder &self, int64_t size) {
                 self.SetSize(size);
             }, (arg("size")), return_self<>())
        .def("SetSize",
             +[](Builder &self, int64_t size, ElementType const &fill) {
                 self.SetSize(size, fill);
             }, (arg("size"), arg("fill")), return_self<>())
        .def("FinalizeAndReset", &Builder::FinalizeAndReset)
        .def("Optimize", +[](ArrayEdit edit) {
            return Builder::Optimize(std::move(edit));
        }, (arg("edit")))
        .staticmethod("Optimize")
        ;
}

#define VT_WRAP_ARRAY_EDIT(unused, elem) \
    VtWrapArrayEdit<VtArrayEdit< VT_TYPE(elem) > >();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_WRAP_ARRAY_EDIT_H

