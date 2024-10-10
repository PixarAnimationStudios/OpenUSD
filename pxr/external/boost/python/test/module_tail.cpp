//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if defined(_WIN32)
# ifdef __MWERKS__
#  pragma ANSI_strict off
# endif
# include <windows.h>
# ifdef __MWERKS__
#  pragma ANSI_strict reset
# endif

# ifdef _MSC_VER
#  include <eh.h> // for _set_se_translator()
#  pragma warning(push)
#  pragma warning(disable:4297)
#  pragma warning(disable:4535)
extern "C" void straight_to_debugger(unsigned int, EXCEPTION_POINTERS*)
{
    throw;
}
extern "C" void (*old_translator)(unsigned, EXCEPTION_POINTERS*)
         = _set_se_translator(straight_to_debugger);
#  pragma warning(pop)
# endif

#endif // _WIN32
