//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/blendShape.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateOffsetsAttr(UsdSkelBlendShape &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateOffsetsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormalOffsetsAttr(UsdSkelBlendShape &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateNormalOffsetsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreatePointIndicesAttr(UsdSkelBlendShape &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePointIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}

static std::string
_Repr(const UsdSkelBlendShape &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdSkel.BlendShape(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdSkelBlendShape(nb::module_ &m)
{
    typedef UsdSkelBlendShape This;

    nb::class_<This, UsdTyped> cls(m, "BlendShape");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetOffsetsAttr",
             &This::GetOffsetsAttr)
        .def("CreateOffsetsAttr",
             &_CreateOffsetsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetNormalOffsetsAttr",
             &This::GetNormalOffsetsAttr)
        .def("CreateNormalOffsetsAttr",
             &_CreateNormalOffsetsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPointIndicesAttr",
             &This::GetPointIndicesAttr)
        .def("CreatePointIndicesAttr",
             &_CreatePointIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace nanobind::detail {
template <typename T>
struct type_caster<PXR_NS::TfSpan<T>> {
    NB_TYPE_CASTER(PXR_NS::TfSpan<T>, const_name("TfSpan"))

    using ElemType = std::remove_const_t<T>;
    using ArrayType = PXR_NS::VtArray<ElemType>;
    std::optional<ArrayType> holder_;

    bool from_python(handle src, uint8_t flags, cleanup_list *cleanup) noexcept {
        if (!src.is_valid() || src.is_none()) {
            return false;
        }
        ArrayType *arr = nullptr;
        if (NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(ArrayType),
                                 src.ptr(), flags, cleanup,
                                 reinterpret_cast<void **>(&arr)) &&
            arr) {
            value = PXR_NS::TfSpan<T>(*arr);
            return true;
        }
        if constexpr (std::is_const_v<T>) {
            if (flags & static_cast<uint8_t>(cast_flags::convert)) {
                make_caster<ArrayType> caster;
                if (caster.from_python(src, flags, cleanup)) {
                    holder_.emplace(static_cast<ArrayType &>(caster));
                    value = PXR_NS::TfSpan<T>(*holder_);
                    return true;
                }
            }
        }
        return false;
    }

    static handle from_cpp(PXR_NS::TfSpan<T> src, rv_policy, cleanup_list *) noexcept {
        ArrayType arr(src.begin(), src.end());
        return type_caster<ArrayType>::from_cpp(arr, rv_policy::copy, nullptr);
    }
};
} // namespace nanobind::detail

namespace {

nb::tuple
_ValidatePointIndices(TfSpan<const int> pointIndices,
                      size_t numPoints)
{
    std::string reason;
    bool valid = UsdSkelBlendShape::ValidatePointIndices(
        pointIndices, numPoints, &reason);
    return nb::make_tuple(valid, reason);
}

WRAP_CUSTOM {

    using This = UsdSkelBlendShape;

    _class
        .def("CreateInbetween", &This::CreateInbetween, nb::arg("name"))
        .def("GetInbetween", &This::GetInbetween, nb::arg("name"))
        .def("HasInbetween", &This::HasInbetween, nb::arg("name"))
        
        .def("GetInbetweens", &This::GetInbetweens,
             nb::rv_policy::copy)
        .def("GetAuthoredInbetweens", &This::GetAuthoredInbetweens,
             nb::rv_policy::copy)

        .def_static("ValidatePointIndices", &_ValidatePointIndices,
             nb::arg("pointIndices"), nb::arg("numPoints"))
        ;
}

}
