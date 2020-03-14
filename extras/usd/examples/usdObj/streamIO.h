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
