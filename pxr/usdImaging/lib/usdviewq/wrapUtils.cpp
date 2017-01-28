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
#include "pxr/usdImaging/usdviewq/utils.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;
using std::vector;

// We return primInfo by unrolling it into a single tuple.  This allows
// python clients to extract the information 40% faster than if we were
// to wrap out UsdviewqUtils::PrimInfo directly.
static tuple
_GetPrimInfo(UsdPrim const &prim, UsdTimeCode time)
{
    UsdviewqUtils::PrimInfo info = UsdviewqUtils::GetPrimInfo(prim, time);
    
    return boost::python::make_tuple(info.hasCompositionArcs,
                                     info.isActive,
                                     info.isImageable,
                                     info.isDefined,
                                     info.isAbstract,
                                     info.isInMaster,
                                     info.isInstance,
                                     info.isVisibilityInherited,
                                     info.visVaries,
                                     info.name,
                                     info.typeName);
}

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

PXR_NAMESPACE_CLOSE_SCOPE

