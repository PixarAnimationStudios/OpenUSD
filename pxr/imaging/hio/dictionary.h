//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_DICTIONARY_H
#define PXR_IMAGING_HIO_DICTIONARY_H

/// \file hio/dictionary.h

#include "pxr/pxr.h"

#include "pxr/base/vt/dictionary.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

VtDictionary
Hio_GetDictionaryFromInput(
    const std::string &input,
    const std::string &filename, 
    std::string *errorStr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
