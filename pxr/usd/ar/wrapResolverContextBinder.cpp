//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/ar/pyResolverContext.h"

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class _PyResolverContextBinder
{
public:
    _PyResolverContextBinder(const ArResolverContext& context)
        : _context(context)
    {
    }

    _PyResolverContextBinder(const _PyResolverContextBinder&) = delete;
    
    _PyResolverContextBinder& 
    operator=(const _PyResolverContextBinder&) = delete;

    void Enter()
    {
        _binder.reset(new ArResolverContextBinder(_context));
    }

    bool Exit(
        nanobind::handle /* exc_type */,
        nanobind::handle /* exc_val  */,
        nanobind::handle /* exc_tb   */)
    {
        _binder.reset(0);
        // Re-raise exceptions.
        return false;
    }

private:
    ArResolverContext _context;
    std::unique_ptr<ArResolverContextBinder> _binder;
};

} // anonymous namespace 

void
wrapResolverContextBinder(nanobind::module_& m)
{
    namespace nb = nanobind;
    typedef _PyResolverContextBinder This;

    nb::class_<This>(m, "ResolverContextBinder")
        .def(nb::init<const ArResolverContext&>())
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit,
             nb::arg().none(), nb::arg().none(), nb::arg().none())
        ;
}
