//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateSkinningMethodAttr(UsdSkelBindingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSkinningMethodAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateGeomBindTransformAttr(UsdSkelBindingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateGeomBindTransformAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateJointsAttr(UsdSkelBindingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateJointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateJointIndicesAttr(UsdSkelBindingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateJointIndicesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateJointWeightsAttr(UsdSkelBindingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateJointWeightsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateBlendShapesAttr(UsdSkelBindingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateBlendShapesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}

static std::string
_Repr(const UsdSkelBindingAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdSkel.BindingAPI(%s)",
        primRepr.c_str());
}

struct UsdSkelBindingAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdSkelBindingAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdSkelBindingAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdSkelBindingAPI::CanApply(prim, &whyNot);
    return UsdSkelBindingAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdSkelBindingAPI(nb::module_ &m)
{
    typedef UsdSkelBindingAPI This;

    UsdSkelBindingAPI_CanApplyResult::WrapNb<UsdSkelBindingAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "BindingAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))

        .def_static("Apply", &This::Apply, nb::arg("prim"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetSkinningMethodAttr",
             &This::GetSkinningMethodAttr)
        .def("CreateSkinningMethodAttr",
             &_CreateSkinningMethodAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetGeomBindTransformAttr",
             &This::GetGeomBindTransformAttr)
        .def("CreateGeomBindTransformAttr",
             &_CreateGeomBindTransformAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetJointsAttr",
             &This::GetJointsAttr)
        .def("CreateJointsAttr",
             &_CreateJointsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetJointIndicesAttr",
             &This::GetJointIndicesAttr)
        .def("CreateJointIndicesAttr",
             &_CreateJointIndicesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetJointWeightsAttr",
             &This::GetJointWeightsAttr)
        .def("CreateJointWeightsAttr",
             &_CreateJointWeightsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetBlendShapesAttr",
             &This::GetBlendShapesAttr)
        .def("CreateBlendShapesAttr",
             &_CreateBlendShapesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetAnimationSourceRel",
             &This::GetAnimationSourceRel)
        .def("CreateAnimationSourceRel",
             &This::CreateAnimationSourceRel)
        
        .def("GetSkeletonRel",
             &This::GetSkeletonRel)
        .def("CreateSkeletonRel",
             &This::CreateSkeletonRel)
        
        .def("GetBlendShapeTargetsRel",
             &This::GetBlendShapeTargetsRel)
        .def("CreateBlendShapeTargetsRel",
             &This::CreateBlendShapeTargetsRel)
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

nb::object
_GetSkeleton(const UsdSkelBindingAPI& binding)
{
    UsdSkelSkeleton skel;
    return binding.GetSkeleton(&skel) ? nb::cast(skel) : nb::none();
}

nb::object
_GetAnimationSource(const UsdSkelBindingAPI& binding)
{
    UsdPrim prim;
    return binding.GetAnimationSource(&prim) ? nb::cast(prim) : nb::none();
}

nb::tuple
_ValidateJointIndices(TfSpan<const int> jointIndices,
                      size_t numJoints)
{
    std::string reason;
    bool valid = UsdSkelBindingAPI::ValidateJointIndices(
        jointIndices, numJoints, &reason);
    return nb::make_tuple(valid, reason);
}

WRAP_CUSTOM {
    using This = UsdSkelBindingAPI;

    _class
        .def("GetJointIndicesPrimvar", &This::GetJointIndicesPrimvar)

        .def("CreateJointIndicesPrimvar", &This::CreateJointIndicesPrimvar,
             nb::arg("constant"), nb::arg("elementSize")=-1)

        .def("GetJointWeightsPrimvar", &This::GetJointWeightsPrimvar)

        .def("CreateJointWeightsPrimvar", &This::CreateJointWeightsPrimvar,
             nb::arg("constant"), nb::arg("elementSize")=-1)

        .def("SetRigidJointInfluence", &This::SetRigidJointInfluence,
             nb::arg("jointIndex"), nb::arg("weight")=1.0f)

        .def("GetSkeleton", &_GetSkeleton)

        .def("GetAnimationSource", &_GetAnimationSource)
        
        .def("GetInheritedSkeleton", &This::GetInheritedSkeleton)
        
        .def("GetInheritedAnimationSource", &This::GetInheritedAnimationSource)

        .def_static("ValidateJointIndices", &_ValidateJointIndices,
             nb::arg("jointIndices"), nb::arg("numJoints"))
        ;
}

} // namespace
