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
#include <bitset>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <complex>
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
#include <tbb/concurrent_hash_map.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/spin_mutex.h>
#include <tbb/spin_rw_mutex.h>
#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pySafePython.h"
#include <frameobject.h>
#include <patchlevel.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED
