//
// Copyright 2023 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/ts/keyFrame.h"
#include "pxr/base/ts/wrapUtils.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/valueFromPython.h"

#include <boost/python.hpp>
#include <boost/function.hpp>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;
using std::string;
using std::vector;


static Ts_AnnotatedBoolResult
_CanSetKnotType(const TsKeyFrame &kf, TsKnotType type)
{
    std::string reason;
    const bool canSet = kf.CanSetKnotType(type, &reason);
    return Ts_AnnotatedBoolResult(canSet, reason);
}


////////////////////////////////////////////////////////////////////////
// Values
//
// For setting and getting values, we want to be able to handle either
// single values, or 2-tuples of values (for dual-valued knots).
//
// Since we wrap these as Python properties, and boost::python doesn't seem
// to let us override the property setter/getters, we handle the
// single-vs-tuple value distinction ourselves.

static boost::python::object
GetValue( TsKeyFrame & kf )
{
    if (kf.GetIsDualValued())
        return boost::python::make_tuple( kf.GetLeftValue(), kf.GetValue() );
    else
        return boost::python::object( kf.GetValue() );
}

static void
SetValue( TsKeyFrame & kf, const boost::python::object & obj )
{
    // Check for a 2-tuple
    boost::python::extract< vector<VtValue> > tupleExtractor( obj );
    if (tupleExtractor.check()) {
        vector< VtValue > vals = tupleExtractor();
        if (vals.size() != 2) {
            TfPyThrowValueError("expected exactly 2 elements for tuple");
        }
        if (!kf.GetIsDualValued()) {
            // Automatically promote to a dual-valued knot.
            kf.SetIsDualValued(true);
        }
        // No change unless we're dual valued (i.e. no slicing).
        if (kf.GetIsDualValued()) {
            kf.SetLeftValue( vals[0] );
            kf.SetValue( vals[1] );
        } else {
            TfPyThrowTypeError("keyframe cannot be made dual-valued");
        }
        return;
    }

    // Check for a single VtValue
    boost::python::extract< VtValue > singleValueExtractor( obj );
    if (singleValueExtractor.check()) {
        VtValue val = singleValueExtractor();
        kf.SetValue( val );
        return;
    }

    // The above extract<VtValue> will always succeed, since a VtValue
    // is templated and can store any type.  This error is here in case
    // that ever changes.
    TfPyThrowTypeError("expected single Vt.Value or pair of Values");
}

static std::string
_Repr(const TsKeyFrame &self)
{
    std::vector<std::string> args;
    args.reserve(11);

    // The first three, respectively, four (when dual-valued) arguments are
    // positional since they are well-established and common to all splines.

    args.push_back(TfPyRepr(self.GetTime()));
    if (self.GetIsDualValued()) {
        // Dual-valued knot
        args.push_back(TfPyRepr(self.GetLeftValue()));
    }
    args.push_back(TfPyRepr(self.GetValue()));
    args.push_back(TfPyRepr(self.GetKnotType()));

    // The remaining arguments are keyword arguments to avoid any ambiguity.

    // Note: We might want to deal with the float types in a more direct way
    // to improve performance.
    if (self.SupportsTangents()) {
        args.push_back("leftSlope=" + TfPyRepr(self.GetLeftTangentSlope()));
        args.push_back("rightSlope=" + TfPyRepr(self.GetRightTangentSlope()));
        args.push_back("leftLen=" + TfPyRepr(self.GetLeftTangentLength()));
        args.push_back("rightLen=" + TfPyRepr(self.GetRightTangentLength()));
    }

    return TF_PY_REPR_PREFIX + "KeyFrame(" + TfStringJoin(args, ", ") + ")";
}


void wrapKeyFrame()
{    
    typedef TsKeyFrame This;

    TfPyWrapEnum<TsSide>();

    TfPyWrapEnum<TsKnotType>();

    TfPyContainerConversions::from_python_sequence<
        std::vector< TsKeyFrame >,
        TfPyContainerConversions::variable_capacity_policy >();

    TfPyContainerConversions::from_python_sequence<
        std::vector< TsKnotType >, 
        TfPyContainerConversions::variable_capacity_policy >();

    to_python_converter< std::vector<TsKnotType>,
        TfPySequenceToPython< std::vector<TsKnotType> > >();

    class_<This>( "KeyFrame", no_init )

        .def( init< const TsTime &,
                    const VtValue &,
                    TsKnotType,
                    const VtValue &,
                    const VtValue &,
                    const TsTime &,
                    const TsTime & >(
             "",
             ( arg("time") = 0.0,
               arg("value") = VtValue(0.0),
               arg("knotType") = TsKnotLinear,
               arg("leftSlope") = VtValue(),
               arg("rightSlope") = VtValue(),
               arg("leftLen") = 0.0,
               arg("rightLen") = 0.0 ) ) )

        .def( init< const TsTime &,
                    const VtValue &,
                    const VtValue &,
                    TsKnotType,
                    const VtValue &,
                    const VtValue &,
                    const TsTime &,
                    const TsTime & >(
             "",
             ( arg("time"),
               arg("leftValue"),
               arg("rightValue"),
               arg("knotType"),
               arg("leftSlope") = VtValue(),
               arg("rightSlope") = VtValue(),
               arg("leftLen") = 0.0,
               arg("rightLen") = 0.0 ) ) ) 

        .def( init<const TsKeyFrame &>() )

        .def("IsEquivalentAtSide", &This::IsEquivalentAtSide)

        .add_property("time",
              &This::GetTime,
              &This::SetTime,
              "The time of this Keyframe.")

        .add_property("value",
              &::GetValue,
              &::SetValue,
              "The value at this Keyframe.  If the keyframe is dual-valued, "
              "this will be a tuple of the (left, right) side values; "
              "otherwise, it will be the single value.  If you assign a "
              "single value to a dual-valued knot, only the right side "
              "will be set, and the left side will remain unchanged.  If "
              "you assign a dual-value (tuple) to a single-valued keyframe "
              "you'll get an exception and the keyframe won't have changed.")
              
        .def("GetValue",
              (VtValue (This::*)(TsSide) const) &This::GetValue,
              "Gets the value at this keyframe on the given side.")

        .def("SetValue",
              (void (This::*)(VtValue, TsSide)) &This::SetValue,
              "Sets the value at this keyframe on the given side.")

        .add_property("knotType",
              &This::GetKnotType,
              &This::SetKnotType,
              "The knot type of this Keyframe.  It controls how the spline "
              "is interpolated around this keyframe.")

        .def("CanSetKnotType", &::_CanSetKnotType,
             "Returns true if the given knot type can be set on this key "
             "frame. If it returns false, it also returns the reason why not. "
             "The reason can be accessed like this: "
             "anim.CanSetKnotType(kf).reasonWhyNot.")

        .add_property("isDualValued",
              &This::GetIsDualValued,
              &This::SetIsDualValued,
              "True if this Keyframe is dual-valued.")

        .add_property("isInterpolatable", &This::IsInterpolatable)

        .add_property("supportsTangents", &This::SupportsTangents)

        .add_property("hasTangents", &This::HasTangents)

        // Slope/length tangent interface
        .add_property("leftSlope",
              make_function(&This::GetLeftTangentSlope,
                            return_value_policy<return_by_value>()),
              &This::SetLeftTangentSlope,
              "The left tangent's slope.")
        .add_property("leftLen",
              &This::GetLeftTangentLength,
              &This::SetLeftTangentLength,
              "The left tangent's length.")
        .add_property("rightSlope",
              make_function(&This::GetRightTangentSlope,
                            return_value_policy<return_by_value>()),
              &This::SetRightTangentSlope,
              "The right tangent's slope.")
        .add_property("rightLen",
              &This::GetRightTangentLength,
              &This::SetRightTangentLength,
              "The right tangent's length.")

        // Tangent symmetry
        .add_property("tangentSymmetryBroken",
              &This::GetTangentSymmetryBroken,
              &This::SetTangentSymmetryBroken,
              "Gets and sets whether symmetry between the left/right "
              "tangents is broken. If true, tangent handles will not "
              "automatically stay symmetric as they are changed.")

        .def("__repr__", ::_Repr)
        .def(self == self)
        .def(self != self)
        ;
    
    VtValueFromPython<TsKeyFrame>();
}
