//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

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

using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapMath(nanobind::module_ &m)
{    

    m.def("IsClose", (bool (*)(double, double, double))GfIsClose);
    m.def("RadiansToDegrees", GfRadiansToDegrees);
    m.def("DegreesToRadians", GfDegreesToRadians);

    m.def("SmoothStep", GfSmoothStep, 
          nanobind::arg("x"), nanobind::arg("min"), nanobind::arg("max"),
          nanobind::arg("slope0") = 0.0,
          nanobind::arg("slope1") = 0.0);

    m.def("Sqr", GfSqr<int>);
    m.def("Sqr", GfSqr<double>);

    m.def("Sqr", GfSqr<GfVec2i>);
    m.def("Sqr", GfSqr<GfVec3i>);
    m.def("Sqr", GfSqr<GfVec2d>);
    m.def("Sqr", GfSqr<GfVec3d>);
    m.def("Sqr", GfSqr<GfVec4d>);
    m.def("Sqr", GfSqr<GfVec2f>);
    m.def("Sqr", GfSqr<GfVec3f>);
    m.def("Sqr", GfSqr<GfVec4f>);

    m.def("Sgn", GfSgn<int>);
    m.def("Sgn", GfSgn<double>);
    
    m.def("Sqrt", (double (*)(double))GfSqrt);
    m.def("Sqrtf", (float (*)(float))GfSqrt, 
        "Sqrtf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Sqrt() to return the square root of f as a float instead of a double.");

    m.def("Exp", (double (*)(double))GfExp);
    m.def("Expf", (float (*)(float))GfExp, 
        "Expf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Exp() to return the exponent of f as a float instead of a double.");

    m.def("Log", (double (*)(double))GfLog);
    m.def("Logf", (float (*)(float))GfLog, 
        "Logf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Log() to return the logarithm of f as a float instead of a double.");

    m.def("Floor", (double (*)(double))GfFloor);
    m.def("Floorf", (float (*)(float))GfFloor, 
        "Floorf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Floor() to return the floor of f as a float instead of a double.");

    m.def("Ceil", (double (*)(double))GfCeil);
    m.def("Ceilf", (float (*)(float))GfCeil, 
        "Ceilf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Ceil() to return the ceiling of f as a float instead of a double.");

    m.def("Abs", (double (*)(double))GfAbs);
    m.def("Absf", (float (*)(float))GfAbs, 
        "Absf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Abs() to return the absolute value of f as a float instead of a double.");

    m.def("Round", (double (*)(double))GfRound);
    m.def("Roundf", (float (*)(float))GfRound, 
        "Roundf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Round() to return the rounded value of f as a float instead of a double.");

    m.def("Pow", (double (*)(double, double))GfPow);
    m.def("Powf", (float (*)(float, float))GfPow, 
        "Powf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Pow() to return the power of f as a float instead of a double.");

    m.def("Clamp", (double (*)(double, double, double))GfClamp);
    m.def("Clampf", (float (*)(float, float, float))GfClamp, 
        "Clampf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Clamp() to return the clamped value of f as a float instead of a double.");
            
    m.def("Mod", (double (*)(double, double))GfMod);
    m.def("Modf", (float (*)(float, float))GfMod, 
        "Modf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Mod() to return the modulus of f as a float instead of a double.");

    m.def("Lerp", GfLerp<double>);
    m.def("Lerpf", GfLerp<float>, 
         "Lerpf(f) -> float\n\n"
        "f : float\n\n"
        "Use instead of Lerp() to return the linear interpolation of f as a float instead of a double.");

    m.def("Lerp", GfLerp<GfVec2i>);
    m.def("Lerp", GfLerp<GfVec3i>);
    m.def("Lerp", GfLerp<GfVec2d>);
    m.def("Lerp", GfLerp<GfVec3d>);
    m.def("Lerp", GfLerp<GfVec4d>);
    m.def("Lerp", GfLerp<GfVec2f>);
    m.def("Lerp", GfLerp<GfVec3f>);
    m.def("Lerp", GfLerp<GfVec4f>);

    m.def("Min", (int (*)(int, int)) GfMin<int>);
    m.def("Min", (int (*)(int, int, int)) GfMin<int>);
    m.def("Min", (int (*)(int, int, int, int)) GfMin<int>);
    m.def("Min", (int (*)(int, int, int, int, int)) GfMin<int>);
    m.def("Min", (double (*)(double, double)) GfMin<double>);
    m.def("Min", (double (*)(double, double, double)) GfMin<double>);
    m.def("Min", (double (*)(double, double, double, double)) GfMin<double>);
    m.def("Min", (double (*)(double, double, double, double, double))
        GfMin<double>);

    m.def("Max", (int (*)(int, int)) GfMax<int>);
    m.def("Max", (int (*)(int, int, int)) GfMax<int>);
    m.def("Max", (int (*)(int, int, int, int)) GfMax<int>);
    m.def("Max", (int (*)(int, int, int, int, int)) GfMax<int>);
    m.def("Max", (double (*)(double, double)) GfMax<double>);
    m.def("Max", (double (*)(double, double, double)) GfMax<double>);
    m.def("Max", (double (*)(double, double, double, double)) GfMax<double>);
    m.def("Max", (double (*)(double, double, double, double, double))
        GfMax<double>);

    m.def("Dot", (double (*)(double, double)) GfDot);
    m.def("CompMult", (double (*)(double, double)) GfCompMult);
    m.def("CompDiv", (double (*)(double, double)) GfCompDiv);

    TfPyContainerConversions::from_python_sequence< std::vector<int>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector<unsigned int>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector<bool>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector<double>, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector< std::vector<int> >, TfPyContainerConversions::variable_capacity_policy>();

    TfPyContainerConversions::from_python_sequence< std::vector< std::vector<double> >, TfPyContainerConversions::variable_capacity_policy>();
 
}
