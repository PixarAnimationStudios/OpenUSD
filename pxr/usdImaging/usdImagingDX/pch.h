
#define TF_MAX_ARITY 7
#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#if defined(ARCH_OS_DARWIN)
   #include <mach/mach_time.h>
#endif
#if defined(ARCH_OS_LINUX)
   #include <x86intrin.h>
#endif
#if defined(ARCH_OS_WINDOWS)
   #ifndef WIN32_LEAN_AND_MEAN
      #define WIN32_LEAN_AND_MEAN
   #endif
#endif

#include <wrl/client.h>
#include <wrl/event.h>
#include "d3d12.h"
//#include "d3dx12.h"
#include <dxgi1_6.h>
#include <DirectxMath.h>
#include <pix.h>

using namespace Microsoft::WRL;