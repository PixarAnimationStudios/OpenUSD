//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/exec/execNode.h"
#include "pxr/usd/exec/execProperty.h"

#include <boost/python.hpp>
#include <boost/python/return_internal_reference.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

// Boost treats a const ptr differently than a non-const ptr, so a custom
// converter is needed to deal with the const-ness
struct ExecNodeConstPtrToPythonConverter
{
    static PyObject* convert(ExecNodeConstPtr execNode) {
        object execNodeObject(ptr(execNode));

        return incref(execNodeObject.ptr());
    }
};

void wrapExecNode()
{
    typedef ExecNode This;
    typedef ExecNodePtr ThisPtr;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeMetadata", ExecNodeMetadata, EXEC_NODE_METADATA_TOKENS
    );
    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeContext", ExecNodeContext, EXEC_NODE_CONTEXT_TOKENS
    );

    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeRole", ExecNodeRole, EXEC_NODE_ROLE_TOKENS
    );

    return_value_policy<copy_const_reference> copyRefPolicy;
    to_python_converter<ExecNodeConstPtr,
                        ExecNodeConstPtrToPythonConverter>();

    class_<This, ThisPtr, bases<NdrNode>, boost::noncopyable>("ExecNode", no_init)
        .def("GetExecInput", &This::GetExecInput,
            return_internal_reference<>())
        .def("GetExecOutput", &This::GetExecOutput,
            return_internal_reference<>())
        .def("GetAssetIdentifierInputNames", &This::GetAssetIdentifierInputNames,
            return_value_policy<TfPySequenceToList>())
        .def("GetDefaultInput", &This::GetDefaultInput,
            return_internal_reference<>())
        .def("GetLabel", &This::GetLabel, copyRefPolicy)
        .def("GetCategory", &This::GetCategory, copyRefPolicy)
        .def("GetHelp", &This::GetHelp)
        .def("GetDepartments", &This::GetDepartments, copyRefPolicy)
        .def("GetPages", &This::GetPages, copyRefPolicy)
        .def("GetPrimvars", &This::GetPrimvars, copyRefPolicy)
        .def("GetAdditionalPrimvarProperties",
            &This::GetAdditionalPrimvarProperties, copyRefPolicy)
        .def("GetImplementationName", &This::GetImplementationName)
        .def("GetRole", &This::GetRole)
        .def("GetPropertyNamesForPage", &This::GetPropertyNamesForPage)
        ;
}
