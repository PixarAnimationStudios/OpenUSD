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
#include "pxr/external/boost/python/iterator.hpp"
#include <list>

using namespace PXR_BOOST_NAMESPACE::python;

typedef std::list<int> list_int;

// Prove that we can handle InputIterators which return rvalues.
class doubling_iterator
{
public:
    using value_type = int;
    using reference = int;
    using pointer = int;
    using difference_type = list_int::iterator::difference_type;
    using iterator_category = list_int::iterator::iterator_category;

    doubling_iterator() = default;
    doubling_iterator(list_int::iterator i) : _iter(i) { }

    bool operator==(doubling_iterator const& rhs) const { return _iter == rhs._iter; }
    bool operator!=(doubling_iterator const& rhs) const { return !(*this == rhs); }

    reference operator*() const { return *_iter * 2; }
    doubling_iterator& operator++() { ++_iter; return *this; }
    doubling_iterator operator++(int) { return doubling_iterator(_iter++); }

private:
    list_int::iterator _iter;
};

typedef std::pair<doubling_iterator,doubling_iterator> list_range2;

list_range2 range2(list_int& x)
{
    return list_range2(
        doubling_iterator(x.begin()), doubling_iterator(x.end()));
}

// We do this in a separate module from iterators_ext (iterators.cpp)
// to work around an MSVC6 linker bug, which causes it to complain
// about a "duplicate comdat" if the input iterator is instantiated in
// the same module with the others.
PXR_BOOST_PYTHON_MODULE(input_iterator)
{
    def("range2", &::range2);
    
    class_<list_range2>("list_range2")
        // We can wrap InputIterators which return by-value
        .def("__iter__"
             , range(&list_range2::first, &list_range2::second))
        ;
}

#include "module_tail.cpp"
