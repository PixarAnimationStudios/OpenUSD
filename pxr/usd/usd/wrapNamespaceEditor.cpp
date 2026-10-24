//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/namespaceEditor.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static
UsdNamespaceEditor::CanApplyResult
_CanApplyEdits(const UsdNamespaceEditor &editor)
{
    return editor.CanApplyEdits();
}
static
std::string 
_FormatWhyNot(const UsdNamespaceEditor::CanApplyResult &result) {
    return TfStringJoin(result.errors, "; ");
}

void wrapUsdNamespaceEditor(nanobind::module_ &m)
{
    using This = UsdNamespaceEditor;

    nb::class_<This> cls(m, "NamespaceEditor");

    nb::class_<This::EditOptions>(cls, "EditOptions")
        .def(nb::init<>())
        .def_rw("allowRelocatesAuthoring", 
            &This::EditOptions::allowRelocatesAuthoring)
    ;

    nb::class_<This::CanApplyResult>(cls, "CanApplyResult")
        .def(nb::init<>())
        .def("__bool__", &This::CanApplyResult::operator bool)
        .def_prop_ro("whyNot", &_FormatWhyNot)
        .def_prop_ro("errors", [](const This::CanApplyResult &r) {
            return r.errors;
        })
        .def_prop_ro("warnings", [](const This::CanApplyResult &r) {
            return r.warnings;
        })
    ;

    cls
        .def(nb::init<const UsdStagePtr &>())
        .def(nb::init<const UsdStagePtr &, const This::EditOptions &>())

        .def("AddDependentStage", &This::AddDependentStage)
        .def("RemoveDependentStage", &This::RemoveDependentStage)
        .def("SetDependentStages", &This::SetDependentStages)

        .def("DeletePrimAtPath", &This::DeletePrimAtPath)
        .def("MovePrimAtPath", &This::MovePrimAtPath)

        .def("DeletePrim", &This::DeletePrim)
        .def("RenamePrim", &This::RenamePrim)
        .def("ReparentPrim", 
            (bool (This::*) (const UsdPrim &, const UsdPrim &))
               &This::ReparentPrim)
        .def("ReparentPrim", 
            (bool (This::*) (const UsdPrim &, const UsdPrim &, 
                const TfToken &)) &This::ReparentPrim)

        .def("DeletePropertyAtPath", &This::DeletePropertyAtPath)
        .def("MovePropertyAtPath", &This::MovePropertyAtPath)

        .def("DeleteProperty", &This::DeleteProperty)
        .def("RenameProperty", &This::RenameProperty)
        .def("ReparentProperty", 
            (bool (This::*) (const UsdProperty &, const UsdPrim &))
               &This::ReparentProperty)
        .def("ReparentProperty", 
            (bool (This::*) (const UsdProperty &, const UsdPrim &, 
                const TfToken &)) &This::ReparentProperty)

        .def("ApplyEdits", &This::ApplyEdits)
        .def("CanApplyEdits", &_CanApplyEdits)
        .def("GetLayersToEdit", &This::GetLayersToEdit)
    ;
}

