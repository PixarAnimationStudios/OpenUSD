//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/python for documentation.

#ifndef PXR_EXTERNAL_BOOST_PYTHON_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python.hpp>

#include "pxr/external/boost/python/noncopyable.hpp"
#include "pxr/external/boost/python/ref.hpp"
#include "pxr/external/boost/python/type.hpp"
#include "pxr/external/boost/python/type_list.hpp"
#else

# include "pxr/external/boost/python/args.hpp"
# include "pxr/external/boost/python/args_fwd.hpp"
# include "pxr/external/boost/python/back_reference.hpp"
# include "pxr/external/boost/python/bases.hpp"
# include "pxr/external/boost/python/borrowed.hpp"
# include "pxr/external/boost/python/call.hpp"
# include "pxr/external/boost/python/call_method.hpp"
# include "pxr/external/boost/python/class.hpp"
# include "pxr/external/boost/python/copy_const_reference.hpp"
# include "pxr/external/boost/python/copy_non_const_reference.hpp"
# include "pxr/external/boost/python/data_members.hpp"
# include "pxr/external/boost/python/def.hpp"
# include "pxr/external/boost/python/default_call_policies.hpp"
# include "pxr/external/boost/python/dict.hpp"
# include "pxr/external/boost/python/docstring_options.hpp"
# include "pxr/external/boost/python/enum.hpp"
# include "pxr/external/boost/python/errors.hpp"
# include "pxr/external/boost/python/exception_translator.hpp"
# include "pxr/external/boost/python/exec.hpp"
# include "pxr/external/boost/python/extract.hpp"
# include "pxr/external/boost/python/handle.hpp"
# include "pxr/external/boost/python/has_back_reference.hpp"
# include "pxr/external/boost/python/implicit.hpp"
# include "pxr/external/boost/python/init.hpp"
# include "pxr/external/boost/python/import.hpp"
# include "pxr/external/boost/python/instance_holder.hpp"
# include "pxr/external/boost/python/iterator.hpp"
# include "pxr/external/boost/python/list.hpp"
# include "pxr/external/boost/python/long.hpp"
# include "pxr/external/boost/python/lvalue_from_pytype.hpp"
# include "pxr/external/boost/python/make_constructor.hpp"
# include "pxr/external/boost/python/make_function.hpp"
# include "pxr/external/boost/python/manage_new_object.hpp"
# include "pxr/external/boost/python/module.hpp"
# include "pxr/external/boost/python/noncopyable.hpp"
# include "pxr/external/boost/python/object.hpp"
# include "pxr/external/boost/python/object_protocol.hpp"
# include "pxr/external/boost/python/object_protocol_core.hpp"
# include "pxr/external/boost/python/opaque_pointer_converter.hpp"
# include "pxr/external/boost/python/operators.hpp"
# include "pxr/external/boost/python/other.hpp"
# include "pxr/external/boost/python/overloads.hpp"
# include "pxr/external/boost/python/pointee.hpp"
# include "pxr/external/boost/python/pure_virtual.hpp"
# include "pxr/external/boost/python/ptr.hpp"
# include "pxr/external/boost/python/raw_function.hpp"
# include "pxr/external/boost/python/ref.hpp"
# include "pxr/external/boost/python/reference_existing_object.hpp"
# include "pxr/external/boost/python/register_ptr_to_python.hpp"
# include "pxr/external/boost/python/return_arg.hpp"
# include "pxr/external/boost/python/return_internal_reference.hpp"
# include "pxr/external/boost/python/return_opaque_pointer.hpp"
# include "pxr/external/boost/python/return_value_policy.hpp"
# include "pxr/external/boost/python/scope.hpp"
# include "pxr/external/boost/python/self.hpp"
# include "pxr/external/boost/python/slice.hpp"
# include "pxr/external/boost/python/slice_nil.hpp"
# include "pxr/external/boost/python/stl_iterator.hpp"
# include "pxr/external/boost/python/str.hpp"
# include "pxr/external/boost/python/to_python_converter.hpp"
# include "pxr/external/boost/python/to_python_indirect.hpp"
# include "pxr/external/boost/python/to_python_value.hpp"
# include "pxr/external/boost/python/tuple.hpp"
# include "pxr/external/boost/python/type.hpp"
# include "pxr/external/boost/python/type_id.hpp"
# include "pxr/external/boost/python/type_list.hpp"
# include "pxr/external/boost/python/with_custodian_and_ward.hpp"

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_HPP
