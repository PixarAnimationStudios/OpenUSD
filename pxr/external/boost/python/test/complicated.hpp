//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_TEST_COMPLICATED_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_TEST_COMPLICATED_HPP
# include <iostream>

# include "simple_type.hpp"

struct complicated
{
    complicated(simple const&, int = 0);
    ~complicated();

    int get_n() const;

    char* s;
    int n;
};

inline complicated::complicated(simple const&s, int _n)
    : s(s.s), n(_n)
{
    std::cout << "constructing complicated: " << this->s << ", " << _n << std::endl;
}

inline complicated::~complicated()
{
    std::cout << "destroying complicated: " << this->s << ", " << n << std::endl;
}

inline int complicated::get_n() const
{
    return n;
}

#endif // PXR_EXTERNAL_BOOST_PYTHON_TEST_COMPLICATED_HPP
