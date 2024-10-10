//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/cleanupEnabler.h"
#include "pxr/external/boost/python.hpp"
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

// This class and its wrapping allow a client to do this:
// 
//    with Sdf.CleanupEnabler() :
//        // some code
//
// Any specs modified in the with block will be removed if they are inert when 
// leaving the 'with' block.
//
class Sdf_PyCleanupEnabler
{
public:
    // We don't create and destory the SdfCleanupEnabler object
    // on construction and destruction, but rather in the Enter/Exit methods.
    // This ties the lifetime of that object to the python 'with' block rather
    // than to the lifetime of this object, which may be garbage collected.
    Sdf_PyCleanupEnabler()
    {
        // Do nothing
    }
    
    // Called by python when entering the 'with' block
    void Enter()
    {
        _state.reset(new SdfCleanupEnabler());
    }
    
    // Called by python when exiting the 'with' block.
    //
    // The three extra arguments are required by the __enter__/__exit__
    // protocol, and can contain the python exception state if the
    // with-statement is exited by an exception.  We ignore them here.
    void Exit(object const &, object const &, object const &)
    {
        _state.reset();
    }

private :
    std::unique_ptr<SdfCleanupEnabler> _state;
};

} // anonymous namespace 

void wrapCleanupEnabler()
{
    typedef Sdf_PyCleanupEnabler This;

    class_<This,noncopyable>("CleanupEnabler",
        init<>())

        // Callbacks for the python 'with' statement
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit)

        ;
}
