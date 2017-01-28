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
#include "pxr/pxr.h"
#include <boost/python/def.hpp>
#include <boost/python.hpp>

#include "pxr/usd/usdUtils/registeredVariantSet.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyEnum.h"

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

#define BOOST_PYTHON_NONE boost::python::object()

void wrapRegisteredVariantSet()
{
    scope registeredVariantSet =
        class_<UsdUtilsRegisteredVariantSet>(
                        "RegisteredVariantSet", 
                        "Info for registered variant set",
                        no_init)
            .def_readonly("name", &UsdUtilsRegisteredVariantSet::name)
            .def_readonly("selectionExportPolicy", &UsdUtilsRegisteredVariantSet::selectionExportPolicy)
    ;
    
    typedef UsdUtilsRegisteredVariantSet::SelectionExportPolicy SelectionExportPolicy;
    enum_<SelectionExportPolicy>("SelectionExportPolicy")
        .value("IfAuthored", SelectionExportPolicy::IfAuthored)
        .value("Always", SelectionExportPolicy::Always)
        .value("Never", SelectionExportPolicy::Never)
    ;
    
}

PXR_NAMESPACE_CLOSE_SCOPE

