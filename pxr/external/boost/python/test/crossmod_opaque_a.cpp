//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Gottfried Ganßauge 2006.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
# include "pxr/external/boost/python/return_opaque_pointer.hpp"
# include "pxr/external/boost/python/def.hpp"
# include "pxr/external/boost/python/module.hpp"
# include "pxr/external/boost/python/return_value_policy.hpp"

typedef struct opaque_ *opaque;

opaque the_op   = ((opaque) 0x47110815);

opaque get() { return the_op; }

PXR_BOOST_PYTHON_OPAQUE_SPECIALIZED_TYPE_ID(opaque_)

namespace bpl = PXR_BOOST_NAMESPACE::python;

PXR_BOOST_PYTHON_MODULE(crossmod_opaque_a)
{
    bpl::def (
        "get",
        &::get,
        bpl::return_value_policy<bpl::return_opaque_pointer>());
}
