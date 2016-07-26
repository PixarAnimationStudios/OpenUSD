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
#include "pxr/base/gf/colorRamp.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

using namespace boost::python;

static std::string
_Repr(const GfColorRamp &r)
{
    return TfStringPrintf(
        "%sColorRamp(%s, %s, %s, %g, %g, %g, %g, %g, %s, %s)",
        TF_PY_REPR_PREFIX.c_str(),
        TfPyRepr(r.GetCMin()).c_str(),
        TfPyRepr(r.GetCMid()).c_str(),
        TfPyRepr(r.GetCMax()).c_str(),
        r.GetMidPos(),
        r.GetWidthMin(),
        r.GetWidthMidIn(),
        r.GetWidthMidOut(),
        r.GetWidthMax(),
        TfPyRepr(r.GetUseColorRamp()).c_str(),
        TfPyRepr(r.GetSwitchable()).c_str());
}

void wrapColorRamp()
{    
    typedef GfColorRamp This;

    class_<This> ( "ColorRamp", "Color ramp, as commonly used in Pixar shaders",
                   init< optional<
                   const GfRGB &,
                   const GfRGB &,
                   const GfRGB &,
                   float, float, float, float, float,
                   bool, bool
                   > >())

        .def(TfTypePythonClass())
        .def(init<bool,
                  optional<const GfRGB &,
                           const GfRGB &,
                           const GfRGB &,
                           float, float, float, float, float,
                           bool> >())

        .def("Eval", &This::Eval, "Evaluate the ramp at x in [0, 1]")
    
        // These GfRGB-valued fields are not wrapped as properties,
        // since it is a mutable type and so usage like this
        //
        //   ramp.cMin.red = 5
        //
        // does not do what callers might expect -- it just modifies a
        // temporary. 
        //
        //.add_property("cMin", 
        //              make_function(&This::GetCMin,
        //              return_value_policy<copy_const_reference>()),
        //              &This::SetCMin)
        //.add_property("cMid", 
        //              make_function(&This::GetCMid,
        //              return_value_policy<copy_const_reference>()),
        //              &This::SetCMid)
        //.add_property("cMax", 
        //              make_function(&This::GetCMax,
        //              return_value_policy<copy_const_reference>()),
        //              &This::SetCMax)

        .def("GetCMin", &This::GetCMin,
             return_value_policy<copy_const_reference>())
        .def("GetCMid", &This::GetCMid,
             return_value_policy<copy_const_reference>())
        .def("GetCMax", &This::GetCMax,
             return_value_policy<copy_const_reference>())
        .def("SetCMin", &This::SetCMin)
        .def("SetCMid", &This::SetCMid)
        .def("SetCMax", &This::SetCMax)

        .add_property("midPos",
                      &This::GetMidPos,
                      &This::SetMidPos)
        .add_property("widthMin",
                      &This::GetWidthMin,
                      &This::SetWidthMin)
        .add_property("widthMax",
                      &This::GetWidthMax,
                      &This::SetWidthMax)
        .add_property("widthMidIn",
                      &This::GetWidthMidIn,
                      &This::SetWidthMidIn)
        .add_property("widthMidOut",
                      &This::GetWidthMidOut,
                      &This::SetWidthMidOut)
        .add_property("switchable",
                      &This::GetSwitchable,
                      &This::SetSwitchable)
        .add_property("useColorRamp",
                      &This::GetUseColorRamp,
                      &This::SetUseColorRamp)
    
        .def( self == self )
        .def( self != self )
        .def( "__repr__", _Repr )
        ;
    
}
