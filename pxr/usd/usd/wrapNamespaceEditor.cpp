//
// Copyright 2023 Pixar
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
#include "pxr/usd/usd/namespaceEditor.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"

#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/scope.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
}
