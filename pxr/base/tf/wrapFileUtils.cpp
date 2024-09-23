//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/external/boost/python/def.hpp"

#include "pxr/base/tf/fileUtils.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapFileUtils() {

    def("TouchFile", &TfTouchFile, (arg("fileName"), arg("create")=bool(false)));
}
