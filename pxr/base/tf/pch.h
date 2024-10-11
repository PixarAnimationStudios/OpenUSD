//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#endif
#if defined(ARCH_OS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <Shlwapi.h>
#include <intrin.h>
#include <io.h>
#include <stringapiset.h>
#endif
#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <ctype.h>
#include <deque>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <inttypes.h>
#include <iosfwd>
#include <iostream>
#include <istream>
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
#include <numeric>
#include <optional>
#include <ostream>
#include <regex>
#include <set>
#include <signal.h>
#include <sstream>
#include <stack>
#include <stdarg.h>
#include <stddef.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#if defined(PXR_PYTHON_SUPPORT_ENABLED) && !defined(PXR_USE_INTERNAL_BOOST_PYTHON)
#include "pxr/external/boost/python/common.hpp"
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
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
#include <boost/python/exception_translator.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/has_back_reference.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
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
#endif // PXR_PYTHON_SUPPORT_ENABLED && !PXR_USE_INTERNAL_BOOST_PYTHON
#include <tbb/concurrent_hash_map.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/spin_mutex.h>
#include <tbb/spin_rw_mutex.h>
#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pySafePython.h"
#include <frameobject.h>
#include <patchlevel.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED
