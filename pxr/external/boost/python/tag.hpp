//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef TAG_DWA2002720_HPP
# define TAG_DWA2002720_HPP

# include <boost/python/detail/prefix.hpp>

namespace boost { namespace python { 

// used only to prevent argument-dependent lookup from finding the
// wrong function in some cases. Cheaper than qualification.
enum tag_t { tag };

}} // namespace boost::python

#endif // TAG_DWA2002720_HPP
