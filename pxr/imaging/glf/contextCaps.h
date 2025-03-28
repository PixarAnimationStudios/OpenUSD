//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_CONTEXT_CAPS_H
#define PXR_IMAGING_GLF_CONTEXT_CAPS_H

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/base/tf/singleton.h"


PXR_NAMESPACE_OPEN_SCOPE


/// \class GlfContextCaps
///
/// This class is intended to be a cache of the capabilites
/// (resource limits and features) of the underlying
/// GL context.
///
/// It serves two purposes.  Firstly to reduce driver
/// transition overhead of querying these values.
/// Secondly to provide access to these values from other
/// threads that don't have the context bound.
///
/// In the event of failure (InitInstance() wasn't called
/// or an issue accessing the GL context), a reasonable
/// set of defaults, based on GL minimums, is provided.
///
///
/// TO DO (bug #124971):
///   - LoadCaps() should be called whenever the context
///     changes.
///   - Provide a mechanism where other Hd systems can
///     subscribe to when the caps changes, so they can
///     update and invalidate.
///
class GlfContextCaps 
{
public:

    /// InitInstance queries the GL context for its capabilities.
    /// It should be called by the application before using systems
    /// that depend on the caps, such as Hydra.  A good example would be
    /// to pair the call to initialize after a call to initialize GL
    GLF_API
    static void InitInstance();

    /// GetInstance() returns the filled capabilities structure.
    /// This function will not populate the caps and will issue a
    /// coding error if it hasn't been filled.
    GLF_API
    static const GlfContextCaps &GetInstance();

    // GL version
    int glVersion;                    // 400 (4.0), 410 (4.1), ...

    // Whether or not we are running with core profile
    bool coreProfile;

    // Max constants
    int maxArrayTextureLayers;

private:
    void _LoadCaps();
    GlfContextCaps();
    ~GlfContextCaps() = default;

    // Disallow copies
    GlfContextCaps(const GlfContextCaps&) = delete;
    GlfContextCaps& operator=(const GlfContextCaps&) = delete;

    friend class TfSingleton<GlfContextCaps>;
};

GLF_API_TEMPLATE_CLASS(TfSingleton<GlfContextCaps>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GLF_CONTEXT_CAPS_H

