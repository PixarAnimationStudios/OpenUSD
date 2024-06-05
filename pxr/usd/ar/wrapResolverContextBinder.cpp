//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <boost/python/class.hpp>
#include <boost/python/object.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include <memory>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class _PyResolverContextBinder
    : public boost::noncopyable
{
public:
    _PyResolverContextBinder(const ArResolverContext& context)
        : _context(context)
    {
    }

    void Enter()
    {
        _binder.reset(new ArResolverContextBinder(_context));
    }

    bool Exit(
        boost::python::object& /* exc_type */,
        boost::python::object& /* exc_val  */,
        boost::python::object& /* exc_tb   */)
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

    class_<This, boost::noncopyable>
        ("ResolverContextBinder", init<const ArResolverContext&>())
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit)
        ;
}
