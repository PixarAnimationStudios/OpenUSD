//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/stageCache.h"

#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include <functional>
#include <memory>
#include <vector>

namespace nb = nanobind;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Expose C++ RAII class as python context manager.
struct Usd_PyStageCacheContext
{
    // Constructor stores off arguments to pass to the factory later.
    template <class Arg>
    explicit Usd_PyStageCacheContext(Arg arg) 
        : _makeContext([arg]() {
                return new UsdStageCacheContext(arg);
            })
        {}

    explicit Usd_PyStageCacheContext(UsdStageCache &cache)
        : _makeContext([&cache]() {
                return new UsdStageCacheContext(cache);
            })
        {}

    // Instantiate the C++ class object and hold it by shared_ptr.
    void __enter__() { _context.reset(_makeContext()); }

    // Drop the shared_ptr.
    void __exit__(nb::args) { _context.reset(); }

private:

    std::shared_ptr<UsdStageCacheContext> _context;
    std::function<UsdStageCacheContext *()> _makeContext;
};

} // anonymous namespace

void wrapUsdStageCacheContext(nb::module_ &m)
{
    TfNbWrapEnum<UsdStageCacheContextBlockType>(m);

    // The use of with_custodian_and_ward(_postcall) below let us bind python
    // object lifetimes together in such a way that we don't get dangling c++
    // references in the c++ objects.  See the boost.python docs for details
    // (google search for boost python with_custodian_and_ward).

    // Must ensure that the returned Wrapper objects below keep their cache
    // argument alive, otherwise the Wrappers could have dangling references to
    // their caches.
    nb::class_<Usd_NonPopulatingStageCacheWrapper>(
        m, "_NonPopulatingStageCacheWrapper", nb::is_weak_referenceable())
        .def("__init__", [](Usd_NonPopulatingStageCacheWrapper *) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        });
    m.def("UseButDoNotPopulateCache", UsdUseButDoNotPopulateCache<UsdStageCache>,
          nb::keep_alive<0, 1>());

    // The constructor needs to ensure that the wrapper arguments be kept alive
    // as long as the context is, to transitively keep their held cache objects
    // alive.
    nb::class_<Usd_PyStageCacheContext>(
        m, "StageCacheContext", nb::is_weak_referenceable())
        .def(nb::init<Usd_NonPopulatingStageCacheWrapper>(),
             nb::keep_alive<1, 2>())
        .def(nb::init<UsdStageCache &>(),
             nb::keep_alive<1, 2>())
        .def(nb::init<UsdStageCacheContextBlockType>())
        .def("__enter__", &Usd_PyStageCacheContext::__enter__)
        .def("__exit__", &Usd_PyStageCacheContext::__exit__)
        ;

}
