//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <boost/python/class.hpp>
#include <boost/python/object.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverScopedCache.h"

#include <memory>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class _PyResolverScopedCache
    : public boost::noncopyable
{
public:
    _PyResolverScopedCache()
    {
    }

    void Enter()
    {
        _scopedCache.reset(new ArResolverScopedCache);
    }

    bool Exit(
        boost::python::object& /* exc_type */,
        boost::python::object& /* exc_val  */,
        boost::python::object& /* exc_tb   */)
    {
        _scopedCache.reset(0);
        // Re-raise exceptions.
        return false;
    }

private:
    std::unique_ptr<ArResolverScopedCache> _scopedCache;
};

} // anonymous namespace 

void
wrapResolverScopedCache()
{
    typedef _PyResolverScopedCache This;

    class_<This, boost::noncopyable>
        ("ResolverScopedCache")
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit)
        ;
}
