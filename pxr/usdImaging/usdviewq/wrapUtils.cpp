//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pxr/usdImaging/usdviewq/utils.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <vector>

using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

// We return primInfo by unrolling it into a single tuple.  This allows
// python clients to extract the information 40% faster than if we were
// to wrap out UsdviewqUtils::PrimInfo directly.
static nb::tuple
_GetPrimInfo(UsdPrim const &prim, UsdTimeCode time)
{
    UsdviewqUtils::PrimInfo info = UsdviewqUtils::GetPrimInfo(prim, time);
    return nb::make_tuple(info.hasCompositionArcs,
                          info.isActive,
                          info.isImageable,
                          info.isDefined,
                          info.isAbstract,
                          info.isInPrototype,
                          info.isInstance,
                          info.supportsGuides,
                          info.supportsDrawMode,
                          info.isVisibilityInherited,
                          info.visVaries,
                          info.name,
                          info.typeName,
                          info.displayName);
}

} // anonymous namespace 

void wrapUtils(nb::module_ &m) {    
    typedef UsdviewqUtils This;

    nb::class_<This>(m, "Utils")
        .def(nb::init<>())
        .def_static("_GetAllPrimsOfType",
            This::_GetAllPrimsOfType)

        .def_static("GetPrimInfo",
             _GetPrimInfo)
        ;
}
