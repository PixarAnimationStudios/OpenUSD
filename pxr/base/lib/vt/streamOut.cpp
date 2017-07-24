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

#include "pxr/pxr.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/streamOut.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

std::ostream &
Vt_StreamOutGeneric(std::type_info const &type,
                    void const *addr,
                    std::ostream &stream)
{
    return stream <<
        TfStringPrintf("<'%s' @ %p>", ArchGetDemangled(type).c_str(), addr);
}

std::ostream &
VtStreamOut(bool const &val, std::ostream &stream)
{
    return stream << static_cast<int>(val);
}

std::ostream &
VtStreamOut(char const &val, std::ostream &stream)
{
    return stream << static_cast<int>(val);
}

std::ostream &
VtStreamOut(unsigned char const &val, std::ostream &stream)
{
    return stream << static_cast<unsigned int>(val);
}

std::ostream &
VtStreamOut(signed char const &val, std::ostream &stream)
{
    return stream << static_cast<int>(val);
}

std::ostream &
VtStreamOut(float const &val, std::ostream &stream)
{
    return stream << TfStreamFloat(val);
}

std::ostream &
VtStreamOut(double const &val, std::ostream &stream)
{
    return stream << TfStreamDouble(val);
}

namespace {

void
_StreamArray(
    VtStreamOutIterator* i,
    size_t size,
    const Vt_Reserved* reserved,
    std::ostream& out)
{
    out << '[';
    for (size_t j = 0; j != size; ++j) {
        if (j) {
            out << ", ";
        }
        i->Next(out);
    }
    out << ']';
}

}

void (*_vtStreamArray)(VtStreamOutIterator*, size_t,
                       const Vt_Reserved*, std::ostream&) = _StreamArray;

VtStreamOutIterator::~VtStreamOutIterator() { }

void
VtStreamOutArray(
    VtStreamOutIterator* i,
    size_t size,
    const Vt_Reserved* reserved,
    std::ostream& out)
{
    _vtStreamArray(i, size, reserved, out);
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
std::ostream &
VtStreamOut(TfPyObjWrapper const &obj, std::ostream &stream)
{
    return stream << TfPyObjectRepr(obj.Get());
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

PXR_NAMESPACE_CLOSE_SCOPE
