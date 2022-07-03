//
// Copyright 2021 Pixar
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

