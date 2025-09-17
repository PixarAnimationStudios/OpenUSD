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

#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/enum.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/scope.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

struct Usd_UsdNamespaceEditorCanEditResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    Usd_UsdNamespaceEditorCanEditResult(bool val, const std::string &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, std::move(msg)) {}
};

template <typename Fn>
Usd_UsdNamespaceEditorCanEditResult
_CallWithAnnotatedResult(const Fn& func) 
{
    std::string whyNot;
    bool result = func(&whyNot);
    return Usd_UsdNamespaceEditorCanEditResult(result, whyNot);
}

static
Usd_UsdNamespaceEditorCanEditResult
_CanApplyEdits(const UsdNamespaceEditor &editor)
{
    return _CallWithAnnotatedResult([&](std::string *whyNot){
        return editor.CanApplyEdits(whyNot);
    });
}

void wrapUsdNamespaceEditor()
{
    using This = UsdNamespaceEditor;

    Usd_UsdNamespaceEditorCanEditResult::Wrap<Usd_UsdNamespaceEditorCanEditResult>(
        "_UsdNamespaceEditorCanEditResult", "whyNot");

    scope s = class_<This>("NamespaceEditor", no_init)
        .def(init<const UsdStagePtr &>())
        .def(init<const UsdStagePtr &, const This::EditOptions &>())

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
    ;

    class_<This::EditOptions>("EditOptions")
        .def(init<>())
        .add_property("allowRelocatesAuthoring", 
            &This::EditOptions::allowRelocatesAuthoring, 
            &This::EditOptions::allowRelocatesAuthoring)
    ;
}
