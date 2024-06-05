//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapStitch.cpp

#include "pxr/pxr.h"
#include <boost/python/def.hpp>

#include "pxr/usd/usdUtils/stitch.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void 
wrapStitch()
{
    def("StitchLayers", 
        (void(*)(const SdfLayerHandle&, const SdfLayerHandle&))
            &UsdUtilsStitchLayers,
        (arg("strongLayer"), arg("weakLayer")));
    def("StitchInfo", 
        (void(*)(const SdfSpecHandle&, const SdfSpecHandle&))
            &UsdUtilsStitchInfo, 
        (arg("strongObj"), arg("weakObj")));
}
