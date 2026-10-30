//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/blendShapeQuery.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/usd/usdSkel/bindingAPI.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

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

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {

nb::tuple
_ComputeSubShapeWeights(const UsdSkelBlendShapeQuery& self,
                        const VtFloatArray& weights)
{
    VtFloatArray subShapeWeights;
    VtUIntArray blendShapeIndices;
    VtUIntArray subShapeIndices;
    
    self.ComputeSubShapeWeights(weights, &subShapeWeights,
                                &blendShapeIndices, &subShapeIndices);

    return nb::make_tuple(
        subShapeWeights, blendShapeIndices, subShapeIndices);
}

template <typename T>
std::vector<T>
_PyListToVector(const nb::sequence& l)
{
    std::vector<T> vec(nb::len(l));
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = nb::cast<T>(l[i]);
    }
    return vec;
}

bool
_ComputeDeformedPoints(const UsdSkelBlendShapeQuery& self,
                       const TfSpan<const float> subShapeWeights,
                       const TfSpan<const unsigned> blendShapeIndices,
                       const TfSpan<const unsigned> subShapeIndices,
                       const nb::sequence& blendShapePointIndices,
                       const nb::sequence& subShapePointOffsets,
                       TfSpan<GfVec3f> points)
{
    return self.ComputeDeformedPoints(
        subShapeWeights, blendShapeIndices, subShapeIndices,
        _PyListToVector<VtIntArray>(blendShapePointIndices),
        _PyListToVector<VtVec3fArray>(subShapePointOffsets), points);
}

} // namespace

void wrapUsdSkelBlendShapeQuery(nb::module_ &m)
{
    using This = UsdSkelBlendShapeQuery;

    nb::class_<This>(m, "BlendShapeQuery")
        .def(nb::init<>())
        .def(nb::init<UsdSkelBindingAPI>())

        .def("__str__", &This::GetDescription)

        .def("GetBlendShape", &This::GetBlendShape)
        .def("GetInbetween", &This::GetInbetween)
        .def("GetBlendShapeIndex", &This::GetBlendShapeIndex)

        .def("GetNumBlendShapes", &This::GetNumBlendShapes)
        .def("GetNumSubShapes", &This::GetNumSubShapes)

        .def("ComputeBlendShapePointIndices",
             &This::ComputeBlendShapePointIndices)

        .def("ComputeSubShapePointOffsets",
             &This::ComputeSubShapePointOffsets)

        .def("ComputeSubShapeWeights", &_ComputeSubShapeWeights)

        .def("ComputeDeformedPoints", &_ComputeDeformedPoints,
             nb::arg("subShapeWeights"),
             nb::arg("blendShapeIndices"),
             nb::arg("subShapeIndices"),
             nb::arg("blendShapePointIndices"),
             nb::arg("subShapePointOffset"),
             nb::arg("points"))
        ;

}
