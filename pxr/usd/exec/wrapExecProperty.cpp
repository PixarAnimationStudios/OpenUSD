//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/exec/execProperty.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

struct TfTokenPairToPythonConverter
{
    static PyObject* convert(const std::pair<TfToken, TfToken>& pair)
    {
        return incref(make_tuple(pair.first, pair.second).ptr());
    }
};

// Boost treats a const ptr differently than a non-const ptr, so a custom
// converter is needed to deal with the const-ness
struct ExecPropertyConstPtrToPythonConverter
{
    static PyObject* convert(ExecPropertyConstPtr execProperty) {
        object execPropertyObject(ptr(execProperty));

        return incref(execPropertyObject.ptr());
    }
};

void wrapExecProperty()
{
    typedef ExecProperty This;
    typedef ExecPropertyPtr ThisPtr;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "PropertyTypes", ExecPropertyTypes, EXEC_PROPERTY_TYPE_TOKENS
    );

    TF_PY_WRAP_PUBLIC_TOKENS(
        "PropertyMetadata", ExecPropertyMetadata, EXEC_PROPERTY_METADATA_TOKENS
    );

    TF_PY_WRAP_PUBLIC_TOKENS(
        "PropertyRole", ExecPropertyRole, EXEC_PROPERTY_ROLE_TOKENS
    );

    to_python_converter<NdrOption, TfTokenPairToPythonConverter>();
    return_value_policy<copy_const_reference> copyRefPolicy;

    to_python_converter<ExecPropertyConstPtr,
                        ExecPropertyConstPtrToPythonConverter>();

    class_<This, ThisPtr, bases<NdrProperty>,
           boost::noncopyable>("ExecProperty", no_init)
        .def("GetDefaultValueAsSdfType", &This::GetDefaultValueAsSdfType,
                copyRefPolicy)
        .def("GetLabel", &This::GetLabel, copyRefPolicy)
        .def("GetHelp", &This::GetHelp)
        .def("GetPage", &This::GetPage, copyRefPolicy)
        .def("GetWidget", &This::GetWidget, copyRefPolicy)
        .def("GetHints", &This::GetHints,
            return_value_policy<TfPyMapToDictionary>())
        .def("GetOptions", &This::GetOptions,
            return_value_policy<TfPySequenceToList>())
        .def("GetImplementationName", &This::GetImplementationName)
        .def("GetValidConnectionTypes", &This::GetValidConnectionTypes,
            copyRefPolicy)
        .def("IsAssetIdentifier", &This::IsAssetIdentifier)
        .def("IsDefaultInput", &This::IsDefaultInput)
        ;
}
