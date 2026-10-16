//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/pxr.h"
#include "pxr/usd/sdf/copyUtils.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/value.h"

#include <functional>

#include "pxr/usd/sdf/pySpec.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{

VtValue
_GetValueForField(
    const SdfLayerHandle& layer, const TfToken& field, 
    const nanobind::handle& obj)
{
    // Need to disambiguate certain types from Python.
    //
    // XXX: See also wrapSpec.cpp. Perhaps this logic should live in
    // pyUtils.h?
    const VtValue& fallback = layer->GetSchema().GetFallback(field);
    if (fallback.IsHolding<TfTokenVector>()) {
        return VtValue(nanobind::cast<TfTokenVector>(obj));
    }

    VtValue v = nanobind::cast<VtValue>(obj);
    if (!fallback.IsEmpty()) {
        v.CastToTypeOf(fallback);
    }
    return v;
}

using Py_SdfShouldCopyValueSig = 
    nanobind::object(
        SdfSpecType, const TfToken&,
        const SdfLayerHandle&, const SdfPath&, bool,
        const SdfLayerHandle&, const SdfPath&, bool);

using Py_SdfShouldCopyValueFn = std::function<Py_SdfShouldCopyValueSig>;

using Py_SdfShouldCopyChildrenSig =
    nanobind::object(
        const TfToken&,
        const SdfLayerHandle&, const SdfPath&, bool,
        const SdfLayerHandle&, const SdfPath&, bool);
 
using Py_SdfShouldCopyChildrenFn = std::function<Py_SdfShouldCopyChildrenSig>;

bool
_ShouldCopyValue(
    const Py_SdfShouldCopyValueFn& pyFunc,
    SdfSpecType specType, const TfToken& field,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    std::optional<VtValue>* value)
{
    nanobind::object result = pyFunc(
        specType, field, 
        srcLayer, srcPath, fieldInSrc, dstLayer, dstPath, fieldInDst);

    if (PyBool_Check(result.ptr())) {
        return nanobind::cast<bool>(result);
    }

    if (nanobind::isinstance<nanobind::tuple>(result)) {
        nanobind::tuple t = nanobind::borrow<nanobind::tuple>(result);
        if (t.size() == 2 && PyBool_Check(t[0].ptr())) {
            const bool status = nanobind::cast<bool>(t[0]);
            *value = _GetValueForField(srcLayer, field, t[1]);
            return status;
        }
    }

    TfPyThrowTypeError(
        "Expected shouldCopyValueFn to return bool or (bool, value)");
    return true;
}

bool
_ShouldCopyChildren(
    const Py_SdfShouldCopyChildrenFn& pyFunc,
    const TfToken& field,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    std::optional<VtValue>* srcChildren,
    std::optional<VtValue>* dstChildren)
{
    nanobind::object result = pyFunc(
        field, srcLayer, srcPath, fieldInSrc, dstLayer, dstPath, fieldInDst);

    if (PyBool_Check(result.ptr())) {
        return nanobind::cast<bool>(result);
    }

    if (nanobind::isinstance<nanobind::tuple>(result)) {
        nanobind::tuple t = nanobind::borrow<nanobind::tuple>(result);
        if (t.size() == 3 && PyBool_Check(t[0].ptr())) {
            const bool status = nanobind::cast<bool>(t[0]);
            *srcChildren = _GetValueForField(srcLayer, field, t[1]);
            *dstChildren = _GetValueForField(srcLayer, field, t[2]);
            return status;
        }
    }

    TfPyThrowTypeError(
        "Expected shouldCopyChildrenFn to return bool or (bool, value, value)");
    return true;
}
   
bool 
Py_SdfCopySpec(
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath,
    const Py_SdfShouldCopyValueFn& shouldCopyValueFn,
    const Py_SdfShouldCopyChildrenFn& shouldCopyChildrenFn)
{
    namespace ph = std::placeholders;

    return SdfCopySpec(
        srcLayer, srcPath, dstLayer, dstPath,
        /* shouldCopyValueFn = */ std::bind(
            _ShouldCopyValue, std::cref(shouldCopyValueFn),
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, ph::_8, 
            ph::_9),
        /* shouldCopyChildrenFn = */ std::bind(
            _ShouldCopyChildren, std::cref(shouldCopyChildrenFn),
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, ph::_8, 
            ph::_9)
        );
}

}

void
wrapCopyUtils(nanobind::module_ &m)
{
    namespace nb = nanobind;

    m.def("CopySpec", 
        (bool(*)(const SdfLayerHandle&, const SdfPath&, 
                 const SdfLayerHandle&, const SdfPath&))
        &SdfCopySpec,
        nb::arg("srcLayer"), nb::arg("srcPath"), nb::arg("dstLayer"), nb::arg("dstPath"));

    m.def("CopySpec", &Py_SdfCopySpec,
        nb::arg("srcLayer"), nb::arg("srcPath"), nb::arg("dstLayer"), nb::arg("dstPath"),
        nb::arg("shouldCopyValueFn"), nb::arg("shouldCopyChildrenFn"));
}
