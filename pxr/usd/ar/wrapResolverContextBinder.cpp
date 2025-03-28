//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/object.hpp"

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
        pxr_boost::python::object& /* exc_type */,
        pxr_boost::python::object& /* exc_val  */,
        pxr_boost::python::object& /* exc_tb   */)
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
wrapResolverContextBinder()
{
    typedef _PyResolverContextBinder This;

    class_<This, noncopyable>
        ("ResolverContextBinder", init<const ArResolverContext&>())
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit)
        ;
}
