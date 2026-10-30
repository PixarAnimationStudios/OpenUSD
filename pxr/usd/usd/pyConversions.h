//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_PY_CONVERSIONS_H
#define PXR_USD_USD_PY_CONVERSIONS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/object.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/usd/sdf/pySpec.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class SdfValueTypeName;
class UsdStage;
class UsdPrimDefinition;
class UsdPrimTypeInfo;
class UsdSchemaRegistry;
class UsdStageCacheContext;
class UsdNamespaceEditor;

// XXX: DEPRECATED.  This function does nothing except convert \p value to
// python.  Do not call, it will be removed.
USD_API
TfPyObjWrapper UsdVtValueToPython(const VtValue &value);

/// Helper for converting a python value to the target Usd/Sdf type, if
/// possible.  Invokes VtValue::CastToTypeOf() to do the conversion, if
/// required.  This internally handles python buffers (e.g. numpy) -> VtArray
/// and some python tuple/list -> VtArray conversions.  If conversion fails,
/// returns a VtValue extracted from the pyVal, which may produce a VtValue
/// holding a python object.
USD_API
VtValue UsdPythonToSdfType(TfPyObjWrapper pyVal,
                           SdfValueTypeName const &targetType);

inline VtValue UsdPythonToSdfType(nanobind::handle pyVal,
                                  SdfValueTypeName const &targetType) {
    return UsdPythonToSdfType(
        TfPyObjWrapper(nanobind::borrow<nanobind::object>(pyVal)),
        targetType);
}

/// Helper for converting a python value to a metadata value for metadata
/// known to the SdfSchema.  Generates a coding error if \p key is unknown
/// to the SdfSchema.
///
/// For dictionary-valued metadata, \p keyPath may be specified as the path
/// in the dictionary we are targeting, so that if the dictionary was registered
/// with a fallback for that dictionary subcomponent, we will convert
/// appropriately to its type.
///
/// \return \c true on successful conversion, which can happen even if
/// the converted \p result is an empty VtValue
USD_API
bool UsdPythonToMetadataValue(const TfToken &key, const TfToken &keyPath, 
                              TfPyObjWrapper pyVal, VtValue *result);

inline bool UsdPythonToMetadataValue(const TfToken &key, const TfToken &keyPath,
                                     nanobind::handle pyVal, VtValue *result) {
    return UsdPythonToMetadataValue(
        key, keyPath,
        TfPyObjWrapper(nanobind::borrow<nanobind::object>(pyVal)),
        result);
}

USD_API
PyObject *Usd_ObjectSubclassConvertToPython(const UsdObject *obj);

USD_API
PyObject *Usd_PrimRangeToPython(const UsdPrimRange &primRange);

USD_API
bool Usd_PrimRangeFromPython(PyObject *obj, UsdPrimRange *out);

USD_API
void Usd_RegisterPrimRangePythonConversions(
    PyObject *(*toPy)(const UsdPrimRange &),
    bool (*fromPy)(PyObject *, UsdPrimRange *));

PXR_NAMESPACE_CLOSE_SCOPE

namespace std {
template <>
struct is_copy_constructible<PXR_NS::UsdStage> : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::UsdStage> : std::false_type {};
template <>
struct is_copy_constructible<PXR_NS::UsdPrimDefinition> : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::UsdPrimDefinition> : std::false_type {};
template <>
struct is_copy_constructible<PXR_NS::UsdPrimTypeInfo> : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::UsdPrimTypeInfo> : std::false_type {};
template <>
struct is_copy_constructible<PXR_NS::UsdSchemaRegistry> : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::UsdSchemaRegistry> : std::false_type {};
template <>
struct is_copy_constructible<PXR_NS::UsdStageCacheContext> : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::UsdStageCacheContext> : std::false_type {};
template <>
struct is_copy_constructible<PXR_NS::UsdNamespaceEditor> : std::false_type {};
} // namespace std

namespace nanobind::detail {

template <>
struct type_caster<PXR_NS::UsdObject>
    : type_caster_base<PXR_NS::UsdObject> {
    static handle from_cpp(const PXR_NS::UsdObject &src, rv_policy,
                           cleanup_list *) noexcept {
        return handle(PXR_NS::Usd_ObjectSubclassConvertToPython(&src));
    }
    static handle from_cpp(const PXR_NS::UsdObject *src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if (!src) {
            return none().release();
        }
        return from_cpp(*src, policy, cleanup);
    }
};

template <>
struct type_caster<PXR_NS::UsdProperty>
    : type_caster_base<PXR_NS::UsdProperty> {
    static handle from_cpp(const PXR_NS::UsdProperty &src, rv_policy,
                           cleanup_list *) noexcept {
        return handle(PXR_NS::Usd_ObjectSubclassConvertToPython(&src));
    }
    static handle from_cpp(const PXR_NS::UsdProperty *src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if (!src) {
            return none().release();
        }
        return from_cpp(*src, policy, cleanup);
    }
};

template <>
struct type_caster<PXR_NS::UsdPrimRange> {
    NB_TYPE_CASTER(PXR_NS::UsdPrimRange, const_name("PrimRange"))

    bool from_python(handle src, uint8_t, cleanup_list *) noexcept {
        return PXR_NS::Usd_PrimRangeFromPython(src.ptr(), &value);
    }

    static handle from_cpp(const PXR_NS::UsdPrimRange &src, rv_policy,
                           cleanup_list *) noexcept {
        return handle(PXR_NS::Usd_PrimRangeToPython(src));
    }
};

} // namespace nanobind::detail

#endif // PXR_USD_USD_PY_CONVERSIONS_H
