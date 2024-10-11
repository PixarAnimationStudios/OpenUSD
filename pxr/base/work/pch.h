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
#include <mach/mach_time.h>
#endif
#if defined(ARCH_OS_LINUX)
#include <unistd.h>
#include <x86intrin.h>
#endif
#if defined(ARCH_OS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <intrin.h>
#endif
#include <algorithm>
#include <any>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <iosfwd>
#include <iterator>
#include <list>
#include <locale>
#include <map>
#include <math.h>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <stdarg.h>
#include <stddef.h>
#include <string>
#include <sys/types.h>
#include <thread>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#if defined(PXR_PYTHON_SUPPORT_ENABLED) && !defined(PXR_USE_INTERNAL_BOOST_PYTHON)
#include "pxr/external/boost/python/common.hpp"
#include <boost/python/def.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/module.hpp>
#if defined(__APPLE__) // Fix breakage caused by Python's pyport.h.
#undef tolower
#undef toupper
#endif
#endif // PXR_PYTHON_SUPPORT_ENABLED && !PXR_USE_INTERNAL_BOOST_PYTHON
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/info.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/parallel_reduce.h>
#include <tbb/spin_rw_mutex.h>
#include <tbb/task.h>
#include <tbb/task_arena.h>
#include <tbb/task_group.h>
