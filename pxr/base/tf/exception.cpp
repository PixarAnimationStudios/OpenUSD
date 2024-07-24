//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/exception.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/stackTrace.h"

PXR_NAMESPACE_OPEN_SCOPE

TfBaseException::TfBaseException(std::string const &message)
    : _message(message)
{
}

TfBaseException::~TfBaseException()
{
}

void
TfBaseException::_ThrowImpl(TfCallContext const &cc,
                            TfBaseException &exc,
                            TfFunctionRef<void ()> thrower,
                            int skipNCallerFrames)
{
    if (TfGetenvBool("TF_FATAL_THROW", false)) {
        TF_FATAL_ERROR("%s (%s thrown)",
                       exc.what(),
                       ArchGetDemangled(typeid(exc)).c_str());
    }

    // Capture a stack trace here, from the throw-point.
    static constexpr size_t ThrowStackDepth = 64;
    // Skip two additional frames, one for this function, and one for
    // ArchGetStackFrames itself.  Also skip one more if we have a call context,
    // since that contains all the information for the final throwing frame
    // itself.
    int skipFrames = 2 + skipNCallerFrames + (cc ? 1 : 0);
    ArchGetStackFrames(ThrowStackDepth, skipFrames, &exc._throwStack);
    exc._callContext = cc;
    thrower();
}

const char *
TfBaseException::what() const noexcept
{
    return _message.c_str();
}

PXR_NAMESPACE_CLOSE_SCOPE

