//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// This is required since our build does not currently support header-only
// libraries.  Without this the headers install okay, but other libs that try to
// use this library will fail to link.  By adding this, an "empty" .so gets
// built and other libs can link it.

int __pxr_boost_python_workaround__;
