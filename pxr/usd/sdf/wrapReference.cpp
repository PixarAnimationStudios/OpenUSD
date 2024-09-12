//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python.hpp"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static string
_Repr(const SdfReference &self)
{
    string args;
    bool useKeywordArgs = false;

    if (!self.GetAssetPath().empty()) {
        args += TfPyRepr(self.GetAssetPath());
    } else {
        useKeywordArgs = true;
    }
    if (!self.GetPrimPath().IsEmpty()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "primPath=" : "") +
            TfPyRepr(self.GetPrimPath());
    } else {
        useKeywordArgs = true;
    }
    if (!self.GetLayerOffset().IsIdentity()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "layerOffset=" : "") +
            TfPyRepr(self.GetLayerOffset());
    } else {
        useKeywordArgs = true;
    }
    // Always use keyword args for custom data (for readability).
    if (!self.GetCustomData().empty()) {
        args += (args.empty() ? "": ", ");
        args += "customData=" + TfPyRepr(self.GetCustomData());
    }

    return TF_PY_REPR_PREFIX + "Reference(" + args + ")";
}

static size_t __hash__(const SdfReference &self)
{
    return TfHash()(self);
}

} // anonymous namespace 

void wrapReference()
{    
    using This = SdfReference;

    // Register conversion for python list <-> vector<SdfReference>
    to_python_converter<
        SdfReferenceVector,
        TfPySequenceToPython<SdfReferenceVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfReferenceVector,
        TfPyContainerConversions::variable_capacity_policy >();

    // Note: Since we have no proxy for Sdf.Reference we wrap it as an
    //       immutable type to avoid confusion about code like this
    //       prim.referenceList.explicitItems[0].assetPath = '//pixar/test.sdf'
    //       This looks like it's updating the assetPath for the prim's
    //       first explicit reference, but would instead modify a temporary
    //       Sdf.Reference object.

    class_<This>( "Reference" )
        .def(init<const string &,
                  const SdfPath &,
                  const SdfLayerOffset &,
                  const VtDictionary &>(
            ( arg("assetPath") = string(),
              arg("primPath") = SdfPath(),
              arg("layerOffset") = SdfLayerOffset(),
              arg("customData") = VtDictionary(0) ) ) )
        .def(init<const This &>())

        .add_property("assetPath",
            make_function(
                &This::GetAssetPath, return_value_policy<return_by_value>()))
        .add_property("primPath",
            make_function(
                &This::GetPrimPath, return_value_policy<return_by_value>()))
        .add_property("layerOffset",
            make_function(
                &This::GetLayerOffset, return_value_policy<return_by_value>()))
        .add_property("customData",
            make_function(
                &This::GetCustomData, return_value_policy<return_by_value>()))

        .def("IsInternal", &This::IsInternal)

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self > self)
        .def(self <= self)
        .def(self >= self)

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)
        ;

}
