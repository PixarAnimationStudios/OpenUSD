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

#include "pxr/base/tf/pyLock.h"

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
        // Do not hold the GIL while constructing the ArResolverScopedCache.
        // The cache may try to construct the resolver and it's possible another
        // thread is already constructing it. We would then block waiting for
        // that construction to complete. Meanwhile, the other thread may need
        // the GIL to load plugins. See GitHub issue #3986.
        TF_PY_ALLOW_THREADS_IN_SCOPE();

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
