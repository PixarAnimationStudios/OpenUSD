//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
