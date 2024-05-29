//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/arch/hash.h"

#include <cstring>

PXR_NAMESPACE_OPEN_SCOPE

void
Tf_HashState::_AppendBytes(char const *bytes, size_t numBytes)
{
    _state = ArchHash64(bytes, numBytes, _state);
    _didOne = true;
}

size_t
TfHashCharPtr::operator()(const char* ptr) const
{
    return TfHash()(static_cast<const void*>(ptr));
}

size_t
TfHashCString::operator()(const char* ptr) const
{
    return ptr ? ArchHash(ptr, std::strlen(ptr)) : 0;
}

bool
TfEqualCString::operator()(const char* lhs, const char* rhs) const
{
    return std::strcmp(lhs, rhs) == 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
