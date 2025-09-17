//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdviewq/utils.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include "pxr/external/boost/python.hpp"

#include <vector>

using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

// We return primInfo by unrolling it into a single tuple.  This allows
// python clients to extract the information 40% faster than if we were
// to wrap out UsdviewqUtils::PrimInfo directly.
static tuple
_GetPrimInfo(UsdPrim const &prim, UsdTimeCode time)
{
    UsdviewqUtils::PrimInfo info = UsdviewqUtils::GetPrimInfo(prim, time);
    return pxr_boost::python::make_tuple(info.hasCompositionArcs,
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

void wrapUtils() {    
    typedef UsdviewqUtils This;

    scope utilsScope = class_<This> ("Utils")
        .def("_GetAllPrimsOfType",
            This::_GetAllPrimsOfType,
            return_value_policy<TfPySequenceToList>())
            .staticmethod("_GetAllPrimsOfType")

        .def("GetPrimInfo",
             _GetPrimInfo)
            .staticmethod("GetPrimInfo")
        ;
}
