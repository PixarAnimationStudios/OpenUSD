//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/make_function.hpp"
#include <string>
#include <cassert>
#include "test_class.hpp"

using namespace PXR_BOOST_NAMESPACE::python;

object new_list()
{
    return list();
}

list listify(object x)
{
    return list(x);
}

object listify_string(char const* s)
{
    return list(s);
}

std::string x_rep(test_class<> const& x)
{
    return "X("  + std::to_string(x.value()) + ")";
}

object apply_object_list(object f, list x)
{
    return f(x);
}

list apply_list_list(object f, list x)
{
    return call<list>(f.ptr(), x);
}

void append_object(list& x, object y)
{
    x.append(y);
}

void append_list(list& x, list const& y)
{
    x.append(y);
}

typedef test_class<> X;

int notcmp(object const& x, object const& y)
{
    return y < x ? -1 : y > x ? 1 : 0;
}

void exercise(list x, object y, object print)
{
    x.append(y);
    x.append(5);
    x.append(X(3));
    
    print("after append:");
    print(x);
    
    print("number of", y, "instances:", x.count(y));
    
    print("number of 5s:", x.count(5));

    x.extend("xyz");
    print("after extend:");
    print(x);
    print("index of", y, "is:", x.index(y));
    print("index of 'l' is:", x.index("l"));
    
    x.insert(4, 666);
    print("after inserting 666:");
    print(x);
    print("inserting with object as index:");
    x.insert(x[x.index(5)], "---");
    print(x);
    
    print("popping...");
    x.pop();
    print(x);
    x.pop(x[x.index(5)]);
    print(x);
    x.pop(x.index(5));
    print(x);

    print("removing", y);
    x.remove(y);
    print(x);
    print("removing", 666);
    x.remove(666);
    print(x);

    print("reversing...");
    x.reverse();
    print(x);

    print("sorted:");
    x.pop(2); // make sorting predictable
    x.pop(2); // remove [1,2] so the list is sortable in py3k
    x.sort();
    print(x);

    print("reverse sorted:");
#if PY_VERSION_HEX >= 0x03000000
    x.sort(*tuple(), **dict(make_tuple(make_tuple("reverse", true))));
#else
    x.sort(&notcmp);
#endif
    print(x);

    list w;
    w.append(5);
    w.append(6);
    w += "hi";
    assert(w[0] == 5);
    assert(w[1] == 6);
    assert(w[2] == 'h');
    assert(w[3] == 'i');
}

PXR_BOOST_PYTHON_MODULE(list_ext)
{
    def("new_list", new_list);
    def("listify", listify);
    def("listify_string", listify_string);
    def("apply_object_list", apply_object_list);
    def("apply_list_list", apply_list_list);
        
    def("append_object", append_object);
    def("append_list", append_list);

    def("exercise", exercise);
    
    class_<X>("X", init<int>())
        .def( "__repr__", x_rep)
        ;
}

#include "module_tail.cpp"
