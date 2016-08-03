//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <boost/python.hpp>
#include <boost/scoped_ptr.hpp>

#include "pxr/usd/sdf/cleanupEnabler.h"

using namespace boost::python;

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
    boost::scoped_ptr<SdfCleanupEnabler> _state;
};

void wrapCleanupEnabler()
{
    typedef Sdf_PyCleanupEnabler This;

    class_<This,boost::noncopyable>("CleanupEnabler",
        init<>())

        // Callbacks for the python 'with' statement
        .def("__enter__", &This::Enter)
        .def("__exit__", &This::Exit)

        ;
}
