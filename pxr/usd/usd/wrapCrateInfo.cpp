//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/crateInfo.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/external/boost/python.hpp"

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdCrateInfo()
{
    scope().attr("CrateInfo") =
        TfPyGetClassObject<SdfCrateInfo>();
    scope().attr("CrateInfo").attr("Section") =
        TfPyGetClassObject<SdfCrateInfo::Section>();
}
