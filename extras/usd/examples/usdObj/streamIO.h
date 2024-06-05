//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTRAS_USD_EXAMPLES_USD_OBJ_STREAM_IO_H
#define PXR_EXTRAS_USD_EXAMPLES_USD_OBJ_STREAM_IO_H

#include "pxr/pxr.h"
#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdObjStream;

/// Read obj data from \a fileName into \a data.  Return true if successful,
/// false otherwise.  If unsuccessful, return an error message in \a error if it
/// is not null.
bool
UsdObjReadDataFromFile(std::string const &fileName,
                       UsdObjStream *stream,
                       std::string *error = 0);

/// Read obj data from \a stream into \a data.  Return true if successful, false
/// otherwise.  If unsuccessful, return an error message in \a error if it is
/// not null.
bool
UsdObjReadDataFromStream(std::istream &input,
                         UsdObjStream *stream,
                         std::string *error = 0);



PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_EXTRAS_USD_EXAMPLES_USD_OBJ_STREAM_IO_H
