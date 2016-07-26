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
#include <boost/python/class.hpp>
#include <boost/python/object.hpp>

#include "pxr/usd/ar/resolverScopedCache.h"

#include <boost/scoped_ptr.hpp>

using namespace boost::python;

namespace
{

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
    boost::scoped_ptr<ArResolverScopedCache> _scopedCache;
};

};

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
