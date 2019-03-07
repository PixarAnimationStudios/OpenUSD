//
// Copyright 2017 Pixar
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
// WARNING: THIS FILE IS GENERATED.  DO NOT EDIT.
//

#define TF_MAX_ARITY 7
#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#if defined(ARCH_OS_DARWIN)
#include <dirent.h>
#include <glob.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>
#include <boost/preprocessor/comparison/greater_equal.hpp>
#include <boost/preprocessor/config/limits.hpp>
#include <mach/mach_time.h>
#endif
#if defined(ARCH_OS_LINUX)
#include <dirent.h>
#include <glob.h>
#include <sys/param.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>
#include <x86intrin.h>
#include <boost/preprocessor/comparison/greater_equal.hpp>
#include <boost/preprocessor/config/limits.hpp>
#endif
#if defined(ARCH_OS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Shlwapi.h>
#include <intrin.h>
#include <io.h>
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_tuple.hpp>
#endif
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <deque>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <inttypes.h>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits.h>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <math.h>
#include <memory>
#include <mutex>
#include <new>
#include <ostream>
#include <set>
#include <signal.h>
#include <sstream>
#include <stdarg.h>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <boost/any.hpp>
#include <boost/assign.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/call_traits.hpp>
#include <boost/compressed_pair.hpp>
#include <boost/function.hpp>
#include <boost/functional/hash.hpp>
#include <boost/functional/hash_fwd.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/remove.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/preprocessor.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/logical/and.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/punctuation/paren.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/seq/filter.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/push_back.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <boost/python.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/borrowed.hpp>
#include <boost/python/call.hpp>
#include <boost/python/class.hpp>
#include <boost/python/converter/from_python.hpp>
#include <boost/python/converter/registered.hpp>
#include <boost/python/converter/registrations.hpp>
#include <boost/python/converter/registry.hpp>
#include <boost/python/converter/rvalue_from_python_data.hpp>
#include <boost/python/converter/to_python_function_type.hpp>
#include <boost/python/def.hpp>
#include <boost/python/def_visitor.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/detail/api_placeholder.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/has_back_reference.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/import.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/module.hpp>
#include <boost/python/object.hpp>
#include <boost/python/object/class_detail.hpp>
#include <boost/python/object/function.hpp>
#include <boost/python/object/iterator.hpp>
#include <boost/python/object_fwd.hpp>
#include <boost/python/object_operators.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/override.hpp>
#include <boost/python/pure_virtual.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/refcount.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/str.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/to_python_value.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/type_id.hpp>
#include <boost/python/wrapper.hpp>
#if defined(__APPLE__) // Fix breakage caused by Python's pyport.h.
#undef tolower
#undef toupper
#endif
#endif // PXR_PYTHON_SUPPORT_ENABLED
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/type_traits.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/has_left_shift.hpp>
#include <boost/type_traits/is_abstract.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/variant.hpp>
#include <tbb/atomic.h>
#include <tbb/concurrent_hash_map.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/spin_mutex.h>
#include <tbb/spin_rw_mutex.h>
#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <boost/python/detail/wrap_python.hpp>
#include <frameobject.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED
