//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/coalescingDiagnosticDelegate.h"

#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <iostream>
#include <string>

namespace std {
template <>
struct is_copy_constructible<PXR_NS::UsdUtilsCoalescingDiagnosticDelegate>
    : std::false_type {};
template <>
struct is_move_constructible<PXR_NS::UsdUtilsCoalescingDiagnosticDelegate>
    : std::false_type {};
}  // namespace std

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

static nb::list
_TakeUncoalescedDiagnostics(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    nb::list result;
    for (auto const& item : d.TakeUncoalescedDiagnostics()) {
        result.append(nb::cast(*item.get())); 
    }
    return result;
}

static nb::list
_TakeCoalescedDiagnostics(UsdUtilsCoalescingDiagnosticDelegate& d)
{
    nb::list result;
    for (auto const& item : d.TakeCoalescedDiagnostics()) {
        result.append(nb::cast(item));
    }
    return result;
}

static nb::list
_GetUnsharedItems(UsdUtilsCoalescingDiagnosticDelegateItem const& d)
{
    nb::list result;
    for (auto const& item : d.unsharedItems) {
        result.append(nb::cast(item));
    }

    return result;
}

void 
wrapCoalescingDiagnosticDelegate(nanobind::module_ &m)
{
     using SharedItem = UsdUtilsCoalescingDiagnosticDelegateSharedItem;
     nb::class_<SharedItem>(m, "CoalescingDiagnosticDelegateSharedItem")
        .def_ro("sourceLineNumber", &SharedItem::sourceLineNumber)
        .def_ro("sourceFileName",   &SharedItem::sourceFileName)
        .def_ro("sourceFunction",   &SharedItem::sourceFunction); 

     using UnsharedItem = UsdUtilsCoalescingDiagnosticDelegateUnsharedItem;
     nb::class_<UnsharedItem>(m, "CoalescingDiagnosticDelegateUnsharedItem")
        .def_ro("context",    &UnsharedItem::context)
        .def_ro("commentary", &UnsharedItem::commentary); 

     using Item = UsdUtilsCoalescingDiagnosticDelegateItem;
     nb::class_<Item>(m, "CoalescingDiagnosticDelegateItem")
        .def_ro("sharedItem",   &Item::sharedItem)
        .def_prop_ro("unsharedItems", &_GetUnsharedItems);

     using This = UsdUtilsCoalescingDiagnosticDelegate;
     nb::class_<This>(m, "CoalescingDiagnosticDelegate")
        .def(nb::init<>())
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
