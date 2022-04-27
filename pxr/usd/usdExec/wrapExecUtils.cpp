//
// Unlicensed 2022 benmalartre
//
#include "pxr/pxr.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/usd/usdExec/execUtils.h"
#include "pxr/usd/usdExec/execInput.h"
#include "pxr/usd/usdExec/execOutput.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static object 
_GetBaseNameAndType(const TfToken &fullName)
{
    const auto &result = UsdExecUtils::GetBaseNameAndType(fullName);
    return make_tuple(result.first, result.second);
}

} // anonymous namespace 

void wrapUsdExecUtils()
{
    UsdExecAttributeVector (*GetValueProducingAttributes_Input)(
        const UsdExecInput &input, bool includeAuthoredValues) = 
            &UsdExecUtils::GetValueProducingAttributes;
    UsdExecAttributeVector (*GetValueProducingAttributes_Output)(
        const UsdExecOutput &output, bool includeAuthoredValues) = 
            &UsdExecUtils::GetValueProducingAttributes;

    scope thisScope = class_<UsdExecUtils>("Utils", no_init)
        .def("GetPrefixForAttributeType", 
            UsdExecUtils::GetPrefixForAttributeType)
        .staticmethod("GetPrefixForAttributeType")

        .def("GetBaseNameAndType", _GetBaseNameAndType)
        .staticmethod("GetBaseNameAndType")

        .def("GetType", UsdExecUtils::GetType)
        .staticmethod("GetType")

        .def("GetFullName", UsdExecUtils::GetFullName)
        .staticmethod("GetFullName")

        .def("GetValueProducingAttributes", GetValueProducingAttributes_Input,
            (arg("input"), arg("outputsOnly")=false))
        .def("GetValueProducingAttributes", GetValueProducingAttributes_Output,
            (arg("output"), arg("outputsOnly")=false))
        .staticmethod("GetValueProducingAttributes")
        ;

}
