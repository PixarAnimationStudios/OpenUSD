//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Stefan Seefeld 2005.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python.hpp"

#include <cassert>
#include <functional>
#include <iostream>


namespace python = PXR_BOOST_NAMESPACE::python;

// An abstract base class
class Base : public boost::noncopyable
{
public:
  virtual ~Base() {};
  virtual std::string hello() = 0;
};

// C++ derived class
class CppDerived : public Base
{
public:
  virtual ~CppDerived() {}
  virtual std::string hello() { return "Hello from C++!";}
};

// Familiar Boost.Python wrapper class for Base
struct BaseWrap : Base, python::wrapper<Base>
{
  virtual std::string hello() 
  {
    return this->get_override("hello")();
  }
};

// Pack the Base class wrapper into a module
PXR_BOOST_PYTHON_MODULE(embedded_hello)
{
  python::class_<BaseWrap, boost::noncopyable> base("Base");
}


void eval_test()
{
  python::object result = python::eval("'abcdefg'.upper()");
  std::string value = python::extract<std::string>(result);
  assert(value == "ABCDEFG");
}

void exec_test()
{
  // Retrieve the main module
  python::object main = python::import("__main__");
  
  // Retrieve the main module's namespace
  python::object global(main.attr("__dict__"));

  // Define the derived class in Python.
  python::object result = python::exec(
    "from embedded_hello import *        \n"
    "class PythonDerived(Base):          \n"
    "    def hello(self):                \n"
    "        return 'Hello from Python!' \n",
    global, global);

  python::object PythonDerived = global["PythonDerived"];

  // Creating and using instances of the C++ class is as easy as always.
  CppDerived cpp;
  assert(cpp.hello() == "Hello from C++!");

  // But now creating and using instances of the Python class is almost
  // as easy!
  python::object py_base = PythonDerived();
  Base& py = python::extract<Base&>(py_base);

  // Make sure the right 'hello' method is called.
  assert(py.hello() == "Hello from Python!");
}

void exec_file_test(std::string const &script)
{
  // Run a python script in an empty environment.
  python::dict global;
  python::object result = python::exec_file(script.c_str(), global, global);

  // Extract an object the script stored in the global dictionary.
  assert(python::extract<int>(global["number"]) ==  42);
}

void exec_test_error()
{
  // Execute a statement that raises a python exception.
  python::dict global;
  python::object result = python::exec("print(unknown) \n", global, global);
}

void exercise_embedding_html()
{
    using namespace PXR_BOOST_NAMESPACE::python;
    /* code from: libs/python/doc/tutorial/doc/tutorial.qbk
       (generates libs/python/doc/tutorial/doc/html/python/embedding.html)
     */
    object main_module = import("__main__");
    object main_namespace = main_module.attr("__dict__");

    object ignored = exec("hello = file('hello.txt', 'w')\n"
                          "hello.write('Hello world!')\n"
                          "hello.close()",
                          main_namespace);
}

void check_pyerr(bool pyerr_expected=false)
{
  if (PyErr_Occurred())
  {
    if (!pyerr_expected) {
      PyErr_Print();
      assert(!"Python Error detected");
    }
    else {
      PyErr_Clear();
    }
  }
  else
  {
    assert(!"A C++ exception was thrown  for which "
            "there was no exception handler registered.");
  }
}

int main(int argc, char **argv)
{
  assert(argc == 2 || argc == 3);
  std::string script = argv[1];

  // Register the module with the interpreter
  if (PyImport_AppendInittab(const_cast<char*>("embedded_hello"),
#if PY_VERSION_HEX >= 0x03000000 
                             PyInit_embedded_hello 
#else 
                             initembedded_hello 
#endif 
                             ) == -1)
  {
    assert(!"Failed to add embedded_hello to the interpreter's "
            "builtin modules");
  }

  // Initialize the interpreter
  Py_Initialize();

  if (python::handle_exception(eval_test)) {
    check_pyerr();
  }
  else if(python::handle_exception(exec_test)) {
    check_pyerr();
  }
  else if (python::handle_exception(std::bind(exec_file_test, script))) {
    check_pyerr();
  }
  
  if (python::handle_exception(exec_test_error))
  {
    check_pyerr(/*pyerr_expected*/ true);
  }
  else
  {
    assert(!"Python exception expected, but not seen.");
  }

  if (argc > 2) {
    // The main purpose is to test compilation. Since this test generates
    // a file and I (rwgk) am uncertain about the side-effects, run it only
    // if explicitly requested.
    exercise_embedding_html();
  }

  // Boost.Python doesn't support Py_Finalize yet.
  // Py_Finalize();
  return 0;
}

// Including this file makes sure
// that on Windows, any crashes (e.g. null pointer dereferences) invoke
// the debugger immediately, rather than being translated into structured
// exceptions that can interfere with debugging.
#include "module_tail.cpp"
