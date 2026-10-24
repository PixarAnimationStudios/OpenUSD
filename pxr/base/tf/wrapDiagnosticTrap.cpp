//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stl.h"

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/vector.h>

namespace {
    
class Tf_PyDiagnosticTrap
{
public:
    TfDiagnosticTrap &__enter__() {
        _trap.reset(new TfDiagnosticTrap {});
        return *_trap;
    }

    void __exit__(nanobind::args) {
        _trap->Dismiss();
    }

private:
    std::unique_ptr<TfDiagnosticTrap> _trap;
};

template <class Ret>
static auto
_MakeCallback(nanobind::object callable)
{
    auto call = [callable](nanobind::object arg) -> Ret {
        nanobind::object res = callable(arg);
        if constexpr (!std::is_void_v<Ret>) {
            return nanobind::cast<Ret>(res);
        }
    };
    return TfOverloads {
        [call](TfError const &e)   { return call(nanobind::cast(e)); },
        [call](TfWarning const &w) { return call(nanobind::cast(w)); },
        [call](TfStatus const &s)  { return call(nanobind::cast(s)); }
    };
}

static nanobind::object
__iter__(TfDiagnosticTrap const &trap)
{
    nanobind::list diagnostics;
    trap.ForEach(TfOverloads {
            [&diagnostics](TfError const &e)   { diagnostics.append(nanobind::cast(e)); },
            [&diagnostics](TfWarning const &w) { diagnostics.append(nanobind::cast(w)); },
            [&diagnostics](TfStatus const &s)  { diagnostics.append(nanobind::cast(s)); }
        });
    return diagnostics.attr("__iter__")();
}

} // anon

void wrapDiagnosticTrap(nanobind::module_& m)
{
    namespace nb = nanobind;
    {
        // for ForEach() and predicates -- see USD-12000 that tracks providing a
        // single shared registration mechanism for these.
        TfPyFunctionFromPython<bool (nb::object)>();
        TfPyFunctionFromPython<void (nb::object)>();

        using This = TfDiagnosticTrap;
        nb::class_<This>(m, "_DiagnosticTrapCtx")
            .def("Dismiss",       &This::Dismiss)
            .def("Clear",         &This::Clear)
            .def("ClearErrors",   &This::ClearErrors)
            .def("ClearWarnings", &This::ClearWarnings)
            .def("ClearStatuses", &This::ClearStatuses)
            .def("IsClean",       &This::IsClean)
            .def("HasErrors",     &This::HasErrors)
            .def("HasWarnings",   &This::HasWarnings)
            .def("HasStatuses",   &This::HasStatuses)
            .def("GetErrors",     [](This const &trap) {
                return std::vector<TfError>(trap.GetErrors().begin(), trap.GetErrors().end());
            })
            .def("GetWarnings",   [](This const &trap) {
                return std::vector<TfWarning>(trap.GetWarnings().begin(), trap.GetWarnings().end());
            })
            .def("GetStatuses",   [](This const &trap) {
                return std::vector<TfStatus>(trap.GetStatuses().begin(), trap.GetStatuses().end());
            })
            .def("ForEach", +[](This const &trap, nb::object callable) {
                trap.ForEach(_MakeCallback<void>(callable));
            })
            .def("HasAnyMatching", +[](This const &trap, nb::object callable) {
                return trap.HasAnyMatching(_MakeCallback<bool>(callable));
            })
            .def("HasAllMatching", +[](This const &trap, nb::object callable) {
                return trap.HasAllMatching(_MakeCallback<bool>(callable));
            })
            .def("CountMatching", +[](This const &trap, nb::object callable) {
                return trap.CountMatching(_MakeCallback<bool>(callable));
            })
            .def("EraseMatching", +[](This &trap, nb::object callable) {
                trap.EraseMatching(_MakeCallback<bool>(callable));
            })
            .def("__iter__",      &__iter__)
            ;
    }
    {
        using This = Tf_PyDiagnosticTrap;
        nb::class_<This>(m, "DiagnosticTrap")
            .def(nb::init<>())
            .def("__enter__", &This::__enter__, nb::rv_policy::reference_internal)
            .def("__exit__",  &This::__exit__)
            ;
    }
}
