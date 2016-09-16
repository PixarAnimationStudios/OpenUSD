//
// Copyright 2016 Pixar
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
#ifndef ARCH_HASH_H
#define ARCH_HASH_H

/// \file arch/hash.h
/// Hash functions.

#include "pxr/base/arch/api.h"
#include "pxr/base/arch/inttypes.h"

/// Hash \a len bytes of \a data.
///
/// To compute a hash value for data that is not contiguous in memory, iterate
/// over all the contiguous blocks of memory and accumulate the hash value by
/// passing it on as \p seed.  Note that this is *not* equivalent to hashing the
/// contiguous pieces as a whole.  Support for that may be added in future.
///
ARCH_API uint32_t ArchHash(const char *data, size_t len);
/// \overload
ARCH_API uint32_t ArchHash(const char *data, size_t len, uint32_t seed);

/// Hash \a len bytes of \a data.
///
/// To compute a hash value for data that is not contiguous in memory, iterate
/// over all the contiguous blocks of memory and accumulate the hash value by
/// passing it on as \p seed.  Note that this is *not* equivalent to hashing the
/// contiguous pieces as a whole.  Support for that may be added in future.
///
ARCH_API uint64_t ArchHash64(const char *data, size_t len);
/// \overload
ARCH_API uint64_t ArchHash64(const char *data, size_t len, uint64_t seed);

#endif // ARCH_HASH_H
