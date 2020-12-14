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
#ifndef PXR_USD_SDF_FILE_IO_H
#define PXR_USD_SDF_FILE_IO_H

#include "pxr/pxr.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

class SdfSpec;

// Helper class for writing out strings for the text file format.
class Sdf_TextOutput
{
public:
    explicit Sdf_TextOutput(std::ostream& out) : _out(out) { }

    Sdf_TextOutput(const Sdf_TextOutput&) = delete;
    const Sdf_TextOutput& operator=(const Sdf_TextOutput&) = delete;

    // Write given \p str to output.
    bool Write(const std::string& str)
    {
        _out << str;
        return true;
    }

    // Write NUL-terminated character string \p str to output.
    bool Write(const char* str)
    {
        _out << str;
        return true;
    }

private:
    std::ostream& _out;
};

// Write the provided \a spec to \a out indented \a indent levels.
bool Sdf_WriteToStream(const SdfSpec &spec, std::ostream& out, size_t indent);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
