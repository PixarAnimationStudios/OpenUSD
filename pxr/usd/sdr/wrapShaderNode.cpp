//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <boost/python.hpp>
#include <boost/python/return_internal_reference.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

// Boost treats a const ptr differently than a non-const ptr, so a custom
// converter is needed to deal with the const-ness
struct SdrShaderNodeConstPtrToPythonConverter
{
    static PyObject* convert(SdrShaderNodeConstPtr shaderNode) {
        object shaderNodeObject(ptr(shaderNode));

        return incref(shaderNodeObject.ptr());
    }
};

void wrapShaderNode()
{
    typedef SdrShaderNode This;
    typedef SdrShaderNodePtr ThisPtr;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeMetadata", SdrNodeMetadata, SDR_NODE_METADATA_TOKENS
    );
    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeContext", SdrNodeContext, SDR_NODE_CONTEXT_TOKENS
    );

    TF_PY_WRAP_PUBLIC_TOKENS(
        "NodeRole", SdrNodeRole, SDR_NODE_ROLE_TOKENS
    );

    return_value_policy<copy_const_reference> copyRefPolicy;
    to_python_converter<SdrShaderNodeConstPtr,
                        SdrShaderNodeConstPtrToPythonConverter>();

    class_<This, ThisPtr, bases<NdrNode>, boost::noncopyable>("ShaderNode", no_init)
        .def("GetShaderInput", &This::GetShaderInput,
            return_internal_reference<>())
        .def("GetShaderOutput", &This::GetShaderOutput,
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
        .def("GetAllVstructNames", &This::GetAllVstructNames)
        ;
}
