/* You can use this file to compile bindings using alternative build systems in
   case the dependence on CMake is problematic. The following instructions
   illustrate how to do this Linux. The commands will need to be updated to
   target other operating systems, but that is beyond the scope of this writeup.
   To investigate platform-dependent subtleties, read the CMake build system
   or run it and examine the commands it generates.

   Step 1: compile libnanobind

   $ NB_DIR=<path to nanobind directory>

   $ CXXFLAGS="-std=c++17 -fvisibility=hidden \
       -DNDEBUG -DNB_COMPACT_ASSERTIONS \
       `python3-config --includes` -fPIC \
       -I $NB_DIR/include \
       -I $NB_DIR/ext/robin_map/include"

   $ g++ src/nb_combined.cpp $CXXFLAGS -O3 -fno-strict-aliasing \
       -ffunction-sections -fdata-sections -c -o libnanobind.o

   Step 2: compile the extension code

   $ g++ my_ext.cpp $CXXFLAGS -Os -c -o my_ext.o

   Step 3: link the extension code with libnanobind

   $ g++ -shared -Wl,-s -Wl,--gc-sections my_ext.o libnanobind.o \
       -o my_ext.cpython-310-x86_64-linux-gnu.so

   A few comments about these compilation parameters:

   - the '-ffunction-sections/-fdata-sections/--gc-sections' parameter ensure
     that unused parts of libnanobind are removed from the compiled extension.

   - the '-fno-strict-aliasing' part is needed by the libnanobind part. This
     flag generally applies to code that uses significant amounts of 'raw'
     CPython API code. You should ensure that libnanobind isn't merged with
     other code via link time optimization (LTO), otherwise you may need to
     specify '-fno-strict-aliasing' at the project level.

   - The '-Wl,-s' parameter strips debug information from the generated shared
     library. Similarly, '-DNDEBUG' and '-DNB_COMPACT_ASSERTIONS' are parameters
     that improve performance and reduce binary size in release builds, but
     which would be omitted in debug builds. '-DNB_COMPACT_ASSERTIONS' replaces
     the message of every failed internal check with a generic one that asks
     the user to rebuild in debug mode. Omit it when building a backend module,
     whose users cannot rebuild it.

   - here, the libnanobind part uses -O3 (optimization for highest peformance),
     while the bindings use -Os (optimization for the smallest size). The
     assumption here is that the code in 'my_ext.cpp' is glue code that isn't
     performance sensitive but which can become very big in a perf.-optimized
     build. I find generally this to be a good default, but you may have other
     preferences. If in doubt, benchmark to see what works best.

   - The suffix of the final shared library depends on the Python version and
     platform. You can query the Python binary about the right extension via
     "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))"

   - Some of the above details change when creating a limited API /
     stable ABI build, which is possible starting with Python 3.12. See the
     CMake build system for details.

   - On macOS, linking should uses a "linker response file" to benefit from the
     latest macOS linker technology ("chained fixups"). For details, see
     https://developer.apple.com/videos/play/wwdc2022/110362/ and
     https://github.com/python/cpython/issues/97524#issuecomment-1458855301.
     The files 'cmake/darwin-ld-cpython.sym' and 'cmake/darwin-ld-pypy.sym'
     provide symbol lists that are needed to do so.

   - The CMake build system can also create a shared build of the 'libnanobind'
     component, which is useful in complex binding projects that ship multiple
     exension libraries. As you can see from the above long list, the current
     build system automates quite a few tedious steps. My suggestion would be to
     not roll your own unless you are 100% convinced that this is really needed.
*/

#include "nb_internals.cpp"
#include "nb_func.cpp"
#include "nb_type.cpp"
#include "nb_enum.cpp"
#include "nb_ndarray.cpp"
#include "nb_static_property.cpp"
#include "nb_datetime.cpp"
#if defined(Py_GIL_DISABLED)
#  include "nb_ft.cpp"
#endif
#include "error.cpp"
#include "common.cpp"
#include "implicit.cpp"
#include "trampoline.cpp"
