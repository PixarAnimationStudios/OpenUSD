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
#ifndef GARCH_GLPLATFORMCONTEXT_WINDOWS_H
#define GARCH_GLPLATFORMCONTEXT_WINDOWS_H

#include "pxr/pxr.h"
#include "pxr/imaging/garch/api.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class GarchWGLContextState {
public:
    /// Construct with the current state.
    GARCH_API
    GarchWGLContextState();

    enum class NullState { nullstate };

    /// Construct with the null state.
    GARCH_API
    GarchWGLContextState(NullState);

    /// Compare for equality.
    GARCH_API
    bool operator==(const GarchWGLContextState& rhs) const;

    /// Returns a hash value for the state.
    GARCH_API
    size_t GetHash() const;

    /// Returns \c true if the context state is valid.
    GARCH_API
    bool IsValid() const;

    /// Make the context current.
    GARCH_API
    void MakeCurrent();

    /// Make no context current.
    GARCH_API
    static void DoneCurrent();

private:
    class _Detail;
    std::shared_ptr<_Detail> _detail;
};

// Hide the platform specific type name behind a common name.
typedef GarchWGLContextState GarchGLPlatformContextState;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // GARCH_GLPLATFORMCONTEXT_WINDOWS_H
