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
#ifndef PXR_BASE_VT_STREAM_OUT_H
#define PXR_BASE_VT_STREAM_OUT_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/tf/enum.h"

#include <iosfwd>
#include <typeinfo>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// Helper that's used to stream a generic string for a type that isn't
// streamable and doesn't provide VtStreamOut.  Inserts a message like
// <'typeName' @ 0xXXXXXXXX>.
VT_API std::ostream &
Vt_StreamOutGeneric(std::type_info const &type,
                    void const *addr,
                    std::ostream &stream);

// Function used in the case that T has a stream insertion operator.
template <class T>
inline auto
Vt_StreamOutImpl(T const &obj, std::ostream &stream, int)
    -> decltype(stream << obj)
{
    return stream << obj;
}

// Function used in the case that T does not have a stream insertion operator.
template <class T>
inline std::ostream &
Vt_StreamOutImpl(T const &obj, std::ostream &stream, long)
{
    return Vt_StreamOutGeneric(
        typeid(T), static_cast<void const *>(&obj), stream);
}

/// VtValue and VtArray make unqualified calls to VtStreamOut when writing
/// values to streams.  Clients may overload VtStreamOut for their own types
/// if desired.
template <class T>
typename std::enable_if<!std::is_enum<T>::value, std::ostream &>::type
VtStreamOut(T const &obj, std::ostream &stream)
{
    // For types that have an operator<< suitable for ostream, we use the
    // traditional int/long 0-argument technique to disambiguate overloads.
    return Vt_StreamOutImpl(obj, stream, 0);
}
template <class EnumT>
typename std::enable_if<std::is_enum<EnumT>::value, std::ostream &>::type
VtStreamOut(EnumT const &e, std::ostream &stream)
{
    return VtStreamOut(TfEnum::GetName(e), stream);
}
VT_API std::ostream &VtStreamOut(bool const &, std::ostream &);
VT_API std::ostream &VtStreamOut(char const &, std::ostream &);
VT_API std::ostream &VtStreamOut(unsigned char const &, std::ostream &);
VT_API std::ostream &VtStreamOut(signed char const &, std::ostream &);
VT_API std::ostream &VtStreamOut(float const &, std::ostream &);
VT_API std::ostream &VtStreamOut(double const &, std::ostream &);

class VtStreamOutIterator {
public:
    VT_API virtual ~VtStreamOutIterator();
    virtual void Next(std::ostream&) = 0;
};

struct Vt_ShapeData;

VT_API void VtStreamOutArray(VtStreamOutIterator*, size_t size,
                             const Vt_ShapeData*, std::ostream&);

#ifdef PXR_PYTHON_SUPPORT_ENABLED
VT_API std::ostream &VtStreamOut(class TfPyObjWrapper const &, std::ostream &);
#endif // PXR_PYTHON_SUPPORT_ENABLED

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_STREAM_OUT_H
