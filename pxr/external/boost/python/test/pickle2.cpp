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

    For simplicity, the __dict__ is not included in the result of
    __getstate__. This is not generally recommended, but a valid
    approach if it is anticipated that the object's __dict__ will
    always be empty. Note that safety guards are provided to catch
    the cases where this assumption is not true.

    pickle3.cpp shows how to include the object's __dict__ in the
    result of __getstate__.

    For more information refer to boost/libs/python/doc/pickle.html.
 */

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/extract.hpp"

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
    getstate(const world& w)
    {
        return PXR_BOOST_NAMESPACE::python::make_tuple(w.get_secret_number());
    }

    static
    void
    setstate(world& w, PXR_BOOST_NAMESPACE::python::tuple state)
    {
        using namespace PXR_BOOST_NAMESPACE::python;
        if (len(state) != 1)
        {
          PyErr_SetObject(PyExc_ValueError,
                          ("expected 1-item tuple in call to __setstate__; got %s"
                           % state).ptr()
              );
          throw_error_already_set();
        }

        long number = extract<long>(state[0]);
        if (number != 42)
            w.set_secret_number(number);
    }
  };

}

PXR_BOOST_PYTHON_MODULE(pickle2_ext)
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
