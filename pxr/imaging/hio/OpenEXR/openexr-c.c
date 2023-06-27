
// The OpenEXR configuration may be set via an external build configuration
#ifdef IMATH_HALF_SAFE_FOR_C
#undef IMATH_HALF_SAFE_FOR_C
#endif
#ifndef EXR_INTERNAL
#define EXR_INTERNAL static
#endif
#ifndef ILMTHREAD_THREADING_ENABLED
#define ILMTHREAD_THREADING_ENABLED
#endif
#ifndef OPENEXR_C_STANDALONE
#define OPENEXR_C_STANDALONE
#endif
#ifndef OPENEXR_EXPORT
#define OPENEXR_EXPORT static
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "OpenEXRCoreUnity.h"
