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
#include "pxr/base/tf/pyResultConversions.h"

#include <functional>
#include <memory>

#include "pxr/external/boost/python.hpp"

#include <vector>

using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
    void __exit__(object, object, object) { _context.reset(); }

private:

    std::shared_ptr<UsdStageCacheContext> _context;
    std::function<UsdStageCacheContext *()> _makeContext;
};

} // anonymous namespace

void wrapUsdStageCacheContext()
{
    TfPyWrapEnum<UsdStageCacheContextBlockType>();

    // The use of with_custodian_and_ward(_postcall) below let us bind python
    // object lifetimes together in such a way that we don't get dangling c++
    // references in the c++ objects.  See the boost.python docs for details
    // (google search for boost python with_custodian_and_ward).

    // Must ensure that the returned Wrapper objects below keep their cache
    // argument alive, otherwise the Wrappers could have dangling references to
    // their caches.
    class_<Usd_NonPopulatingStageCacheWrapper>(
        "_NonPopulatingStageCacheWrapper", no_init);
    def("UseButDoNotPopulateCache", UsdUseButDoNotPopulateCache<UsdStageCache>,
        with_custodian_and_ward_postcall<0, 1>());

    // The constructor needs to ensure that the wrapper arguments be kept alive
    // as long as the context is, to transitively keep their held cache objects
    // alive.
    class_<Usd_PyStageCacheContext>("StageCacheContext", no_init)
        .def(init<Usd_NonPopulatingStageCacheWrapper>()[
                 with_custodian_and_ward<1, 2>()])
        .def(init<UsdStageCache &>()[
                 with_custodian_and_ward<1, 2>()])
        .def(init<UsdStageCacheContextBlockType>())
        .def("__enter__", &Usd_PyStageCacheContext::__enter__)
        .def("__exit__", &Usd_PyStageCacheContext::__exit__)
        ;

}
