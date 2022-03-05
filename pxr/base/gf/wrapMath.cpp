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
#include <boost/python/def.hpp>

#include "pxr/pxr.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include "pxr/base/gf/math.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4d.h"


PXR_NAMESPACE_USING_DIRECTIVE

void wrapMath()
{    

    boost::python::def("IsClose", (bool (*)(double, double, double))GfIsClose);
    boost::python::def("RadiansToDegrees", GfRadiansToDegrees);
    boost::python::def("DegreesToRadians", GfDegreesToRadians);

    boost::python::def("Sqr", GfSqr<double>);
    boost::python::def("Sqr", GfSqr<int>);

    boost::python::def("Sqr", GfSqr<GfVec2i>);
    boost::python::def("Sqr", GfSqr<GfVec3i>);
    boost::python::def("Sqr", GfSqr<GfVec2f>);
    boost::python::def("Sqr", GfSqr<GfVec3f>);
    boost::python::def("Sqr", GfSqr<GfVec4f>);
    boost::python::def("Sqr", GfSqr<GfVec2d>);
    boost::python::def("Sqr", GfSqr<GfVec3d>);
    boost::python::def("Sqr", GfSqr<GfVec4d>);

    boost::python::def("Sgn", GfSgn<double>);
    boost::python::def("Sgn", GfSgn<int>);
    
    boost::python::def("Sqrt", (double (*)(double))GfSqrt);
    boost::python::def("Sqrtf", (float (*)(float))GfSqrt, 
        "Sqrtf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Sqrt() to return the square root of f as a float instead of a double.");

    boost::python::def("Exp", (double (*)(double))GfExp);
    boost::python::def("Expf", (float (*)(float))GfExp, 
        "Expf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Exp() to return the exponent of f as a float instead of a double.");

    boost::python::def("Log", (double (*)(double))GfLog);
    boost::python::def("Logf", (float (*)(float))GfLog, 
        "Logf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Log() to return the logarithm of f as a float instead of a double.");

    boost::python::def("Floor", (double (*)(double))GfFloor);
    boost::python::def("Floorf", (float (*)(float))GfFloor, 
        "Floorf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Floor() to return the floor of f as a float instead of a double.");

    boost::python::def("Ceil", (double (*)(double))GfCeil);
    boost::python::def("Ceilf", (float (*)(float))GfCeil, 
        "Ceilf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Ceil() to return the ceiling of f as a float instead of a double.");

    boost::python::def("Abs", (double (*)(double))GfAbs);
    boost::python::def("Absf", (float (*)(float))GfAbs, 
        "Absf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Abs() to return the absolute value of f as a float instead of a double.");

    boost::python::def("Round", (double (*)(double))GfRound);
    boost::python::def("Roundf", (float (*)(float))GfRound, 
        "Roundf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Round() to return the rounded value of f as a float instead of a double.");

    boost::python::def("Pow", (double (*)(double, double))GfPow);
    boost::python::def("Powf", (float (*)(float, float))GfPow, 
        "Powf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Pow() to return the power of f as a float instead of a double.");

    boost::python::def("Clamp", (double (*)(double, double, double))GfClamp);
    boost::python::def("Clampf", (float (*)(float, float, float))GfClamp, 
        "Clampf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Clamp() to return the clamped value of f as a float instead of a double.");
            
    boost::python::def("Mod", (double (*)(double, double))GfMod);
    boost::python::def("Modf", (float (*)(float, float))GfMod, 
        "Modf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Mod() to return the modulus of f as a float instead of a double.");

    boost::python::def("Lerp", GfLerp<double>);
    boost::python::def("Lerpf", GfLerp<float>, 
         "Lerpf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Lerp() to return the linear interpolation of f as a float instead of a double.");

    boost::python::def("Lerp", GfLerp<GfVec2i>);
    boost::python::def("Lerp", GfLerp<GfVec3i>);
    boost::python::def("Lerp", GfLerp<GfVec2f>);
    boost::python::def("Lerp", GfLerp<GfVec3f>);
    boost::python::def("Lerp", GfLerp<GfVec4f>);
    boost::python::def("Lerp", GfLerp<GfVec2d>);
    boost::python::def("Lerp", GfLerp<GfVec3d>);
    boost::python::def("Lerp", GfLerp<GfVec4d>);

    boost::python::def("Min", (double (*)(double, double)) GfMin<double>);
    boost::python::def("Min", (double (*)(double, double, double)) GfMin<double>);
    boost::python::def("Min", (double (*)(double, double, double, double)) GfMin<double>);
    boost::python::def("Min", (double (*)(double, double, double, double, double))
        GfMin<double>);
    boost::python::def("Min", (int (*)(int, int)) GfMin<int>);
    boost::python::def("Min", (int (*)(int, int, int)) GfMin<int>);
    boost::python::def("Min", (int (*)(int, int, int, int)) GfMin<int>);
    boost::python::def("Min", (int (*)(int, int, int, int, int)) GfMin<int>);

    boost::python::def("Max", (double (*)(double, double)) GfMax<double>);
    boost::python::def("Max", (double (*)(double, double, double)) GfMax<double>);
    boost::python::def("Max", (double (*)(double, double, double, double)) GfMax<double>);
    boost::python::def("Max", (double (*)(double, double, double, double, double))
        GfMax<double>);
    boost::python::def("Max", (int (*)(int, int)) GfMax<int>);
    boost::python::def("Max", (int (*)(int, int, int)) GfMax<int>);
    boost::python::def("Max", (int (*)(int, int, int, int)) GfMax<int>);
    boost::python::def("Max", (int (*)(int, int, int, int, int)) GfMax<int>);

    boost::python::def("Dot", (double (*)(double, double)) GfDot);
    boost::python::def("CompMult", (double (*)(double, double)) GfCompMult);
    boost::python::def("CompDiv", (double (*)(double, double)) GfCompDiv);

    TfPyContainerConversions::from_python_sequence< std::vector<int>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector<unsigned int>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector<bool>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector<double>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector< std::vector<int> >, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector< std::vector<double> >, TfPyContainerConversions::variable_capacity_policy>();
 
}
