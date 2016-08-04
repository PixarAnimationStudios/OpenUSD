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
#ifndef GARCH_GLPLATFORMCONTEXTW_H
#define GARCH_GLPLATFORMCONTEXTW_H

#include "pxr/base/arch/defines.h"
#include "pxr/imaging/garch/api.h"

#include <Windows.h>

class GarchGLWContextState {
public:
    /// Construct with the current state.
    GARCH_API
    GarchGLWContextState();

    /// Construct with the given state.
    GARCH_API
	GarchGLWContextState(HDC, HDC, HGLRC);

    /// Compare for equality.
    GARCH_API
    bool operator==(const GarchGLWContextState& rhs) const;

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

public:
	HDC display;
	HDC drawable;
	HGLRC context;

private:
    bool _defaultCtor;
};

// Hide the platform specific type name behind a common name.
typedef GarchGLWContextState GarchGLPlatformContextState;

#endif  // GARCH_GLPLATFORMCONTEXTW_H
