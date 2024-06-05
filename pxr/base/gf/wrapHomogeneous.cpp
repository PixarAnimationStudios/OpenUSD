//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <boost/python/def.hpp>

#include "pxr/pxr.h"
#include "pxr/base/gf/homogeneous.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapHomogeneous()
{    
    def("GetHomogenized", (GfVec4d (*)(const GfVec4d &)) GfGetHomogenized);
    def("GetHomogenized", (GfVec4f (*)(const GfVec4f &)) GfGetHomogenized);

    def("HomogeneousCross", (GfVec4d (*)(const GfVec4d &, const GfVec4d &)) GfHomogeneousCross);
    def("HomogeneousCross", (GfVec4f (*)(const GfVec4f &, const GfVec4f &)) GfHomogeneousCross);

    def("Project", (GfVec3d (*)(const GfVec4d &)) GfProject);
    def("Project", (GfVec3f (*)(const GfVec4f &)) GfProject);
}
