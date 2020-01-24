//
// Copyright 2017 Pixar
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
#include "pxr/usd/usdUtils/coalescingDiagnosticDelegate.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <iostream>
#include <string>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/tuple.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

static void
_DumpCoalescedDiagnosticsToStdout(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    d.DumpCoalescedDiagnostics(std::cout);
}

static void
_DumpCoalescedDiagnosticsToStderr(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    d.DumpCoalescedDiagnostics(std::cerr);
}

static void
_DumpUncoalescedDiagnosticsToStdout(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    d.DumpUncoalescedDiagnostics(std::cout);
}

static void
_DumpUncoalescedDiagnosticsToStderr(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    d.DumpUncoalescedDiagnostics(std::cerr);
}

static boost::python::list
_TakeUncoalescedDiagnostics(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    boost::python::list result;
    for (auto const& item : d.TakeUncoalescedDiagnostics()) {
        result.append(*item.get()); 
    }
    return result;
}

static boost::python::list
_TakeCoalescedDiagnostics(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    boost::python::list result;
    for (auto const& item : d.TakeCoalescedDiagnostics()) {
        result.append(item);
    }
    return result;
}

static boost::python::list
_GetUnsharedItems(UsdUtilsCoalescingDiagnosticDelegateItem const& d)
{
    boost::python::list result;
    for (auto const& item : d.unsharedItems) {
        result.append(item);
    }

    return result;
}

void 
wrapCoalescingDiagnosticDelegate()
{
     using SharedItem = UsdUtilsCoalescingDiagnosticDelegateSharedItem;
     class_<SharedItem>("CoalescingDiagnosticDelegateSharedItem", no_init)
        .add_property("sourceLineNumber", &SharedItem::sourceLineNumber)
        .add_property("sourceFileName",   &SharedItem::sourceFileName)
        .add_property("sourceFunction",   &SharedItem::sourceFunction); 

     using UnsharedItem = UsdUtilsCoalescingDiagnosticDelegateUnsharedItem;
     class_<UnsharedItem>("CoalescingDiagnosticDelegateUnsharedItem", no_init)
        .add_property("context",    &UnsharedItem::context)
        .add_property("commentary", &UnsharedItem::commentary); 

     using Item = UsdUtilsCoalescingDiagnosticDelegateItem;
     class_<Item>("CoalescingDiagnosticDelegateItem", no_init)
        .add_property("sharedItem",   &Item::sharedItem)
        .add_property("unsharedItems", &_GetUnsharedItems);

     using This = UsdUtilsCoalescingDiagnosticDelegate;
     class_<This, boost::noncopyable>("CoalescingDiagnosticDelegate")
        .def("DumpCoalescedDiagnosticsToStdout", 
             &_DumpCoalescedDiagnosticsToStdout)
        .def("DumpUncoalescedDiagnostics", 
             &_DumpUncoalescedDiagnosticsToStdout)
        .def("DumpCoalescedDiagnosticsToStderr", 
             &_DumpCoalescedDiagnosticsToStderr)
        .def("DumpUncoalescedDiagnostics", 
             &_DumpUncoalescedDiagnosticsToStderr)
        .def("TakeCoalescedDiagnostics", &_TakeCoalescedDiagnostics)
        .def("TakeUncoalescedDiagnostics", &_TakeUncoalescedDiagnostics);
}
