//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Ralf W. Grosse-Kunstleve 2002-2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
    This example shows how to make an Extension Class "pickleable".

    The world class below contains member data (secret_number) that
    cannot be restored by any of the constructors. Therefore it is
    necessary to provide the __getstate__/__setstate__ pair of pickle
    interface methods.

    The object's __dict__ is included in the result of __getstate__.
    This requires more code (compare with pickle2.cpp), but is
    unavoidable if the object's __dict__ is not always empty.

    For more information refer to boost/libs/python/doc/pickle.html.
 */

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/back_reference.hpp"

namespace PXR_BOOST_NAMESPACE_python_test {

  // A friendly class.
  class world
  {
    public:
      world(const std::string& _country) : secret_number(0) {
        this->country = _country;
      }
      std::string greet() const { return "Hello from " + country + "!"; }
      std::string get_country() const { return country; }
      void set_secret_number(int number) { secret_number = number; }
      int get_secret_number() const { return secret_number; }
    private:
      std::string country;
      int secret_number;
  };

  struct world_pickle_suite : PXR_BOOST_NAMESPACE::python::pickle_suite
  {
    static
    PXR_BOOST_NAMESPACE::python::tuple
    getinitargs(const world& w)
    {
        return PXR_BOOST_NAMESPACE::python::make_tuple(w.get_country());
    }

    static
    PXR_BOOST_NAMESPACE::python::tuple
    getstate(PXR_BOOST_NAMESPACE::python::object w_obj)
    {
        world const& w = PXR_BOOST_NAMESPACE::python::extract<world const&>(w_obj)();

        return PXR_BOOST_NAMESPACE::python::make_tuple(
            w_obj.attr("__dict__"),
            w.get_secret_number());
    }

    static
    void
    setstate(PXR_BOOST_NAMESPACE::python::object w_obj, PXR_BOOST_NAMESPACE::python::tuple state)
    {
        using namespace PXR_BOOST_NAMESPACE::python;
        world& w = extract<world&>(w_obj)();

        if (len(state) != 2)
        {
          PyErr_SetObject(PyExc_ValueError,
                          ("expected 2-item tuple in call to __setstate__; got %s"
                           % state).ptr()
              );
          throw_error_already_set();
        }

        // restore the object's __dict__
        dict d = extract<dict>(w_obj.attr("__dict__"))();
        d.update(state[0]);

        // restore the internal state of the C++ object
        long number = extract<long>(state[1]);
        if (number != 42)
            w.set_secret_number(number);
    }

    static bool getstate_manages_dict() { return true; }
  };

}

PXR_BOOST_PYTHON_MODULE(pickle3_ext)
{
    using namespace PXR_BOOST_NAMESPACE_python_test;
    PXR_BOOST_NAMESPACE::python::class_<world>(
        "world", PXR_BOOST_NAMESPACE::python::init<const std::string&>())
        .def("greet", &world::greet)
        .def("get_secret_number", &world::get_secret_number)
        .def("set_secret_number", &world::set_secret_number)
        .def_pickle(world_pickle_suite())
        ;
}
