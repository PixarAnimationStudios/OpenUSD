//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/sdf/reference.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python.hpp>
#include <boost/function.hpp>

#include <string>

using namespace boost::python;
using std::string;


static string
_Repr(const SdfReference &self)
{
    string args;
    bool useKeywordArgs = false;

    if (not self.GetAssetPath().empty()) {
        args += TfPyRepr(self.GetAssetPath());
    } else {
        useKeywordArgs = true;
    }
    if (not self.GetPrimPath().IsEmpty()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "primPath=" : "") +
            TfPyRepr(self.GetPrimPath());
    } else {
        useKeywordArgs = true;
    }
    if (not self.GetLayerOffset().IsIdentity()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "layerOffset=" : "") +
            TfPyRepr(self.GetLayerOffset());
    } else {
        useKeywordArgs = true;
    }
    // Always use keyword args for custom data (for readability).
    if (not self.GetCustomData().empty()) {
        args += (args.empty() ? "": ", ");
        args += "customData=" + TfPyRepr(self.GetCustomData());
    }

    return TF_PY_REPR_PREFIX + "Reference(" + args + ")";
}

void wrapReference()
{    
    typedef SdfReference This;

    // Register conversion for python list <-> vector<SdfReference>
    to_python_converter<
        SdfReferenceVector,
        TfPySequenceToPython<SdfReferenceVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfReferenceVector,
        TfPyContainerConversions::variable_capacity_policy >();

    // Note: Since we have no proxy for Sdf.Reference we wrap it as an
    //       immutable type to avoid confusion about code like this
    //       prim.referenceList.explicitItems[0].assetPath = '//menv30/test.menva'
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

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self > self)
        .def(self <= self)
        .def(self >= self)

        .def("__repr__", ::_Repr)

        ;

}
