//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/object.hpp"

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverScopedCache.h"

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

class _PyResolverScopedCache
{
public:
    _PyResolverScopedCache()
    {
    }

    _PyResolverScopedCache(const _PyResolverScopedCache&) = delete;
    _PyResolverScopedCache& operator=(const _PyResolverScopedCache&) = delete;

    void Enter()
    {
        _scopedCache.reset(new ArResolverScopedCache);
    }

    bool Exit(
        pxr_boost::python::object& /* exc_type */,
        pxr_boost::python::object& /* exc_val  */,
        pxr_boost::python::object& /* exc_tb   */)
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

    class_<This, noncopyable>
        ("ResolverScopedCache")
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit)
        ;
}
