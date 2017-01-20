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
#ifndef VT_STREAMOUT_H
#define VT_STREAMOUT_H

#include "pxr/pxr.h"
#include "pxr/base/tf/enum.h"

#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/has_left_shift.hpp>
#include <boost/utility/enable_if.hpp>

#include <iosfwd>
#include <string>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

// Helper that's used to stream a generic string for a type that isn't
// streamable and doesn't provide VtStreamOut.  Inserts a message like
// <'typeName' @ 0xXXXXXXXX>.
std::ostream &
Vt_StreamOutGeneric(std::type_info const &type,
                    void const *addr,
                    std::ostream &stream);

// Package up a type traits test for stream insertion operator.
template <class T>
struct Vt_IsOutputStreamable : boost::has_left_shift<
    std::ostream &, /* << */ T const &, /* -> */ std::ostream &> {};

// Function used in the case that T has a stream insertion operator.
template <class T>
typename boost::enable_if<Vt_IsOutputStreamable<T>, std::ostream &>::type
Vt_StreamOutImpl(T const &obj, std::ostream &stream)
{
    return stream << obj;
}

// Function used in the case that T does not have a stream insertion operator.
template <class T>
typename boost::disable_if<Vt_IsOutputStreamable<T>, std::ostream &>::type
Vt_StreamOutImpl(T const &obj, std::ostream &stream)
{
    return Vt_StreamOutGeneric(
        typeid(T), static_cast<void const *>(&obj), stream);
}

/// VtValue and VtArray make unqualified calls to VtStreamOut when writing
/// values to streams.  Clients may overload VtStreamOut for their own types
/// if desired.
template <class T>
typename boost::disable_if<boost::is_enum<T>, std::ostream &>::type
VtStreamOut(T const &obj, std::ostream &stream)
{
    return Vt_StreamOutImpl(obj, stream);
}
template <class EnumT>
typename boost::enable_if<boost::is_enum<EnumT>, std::ostream &>::type
VtStreamOut(EnumT const &e, std::ostream &stream)
{
    return VtStreamOut(TfEnum::GetName(e), stream);
}
std::ostream &VtStreamOut(bool const &, std::ostream &);
std::ostream &VtStreamOut(char const &, std::ostream &);
std::ostream &VtStreamOut(unsigned char const &, std::ostream &);
std::ostream &VtStreamOut(signed char const &, std::ostream &);
std::ostream &VtStreamOut(float const &, std::ostream &);
std::ostream &VtStreamOut(double const &, std::ostream &);
std::ostream &VtStreamOut(class TfPyObjWrapper const &, std::ostream &);

class VtStreamOutIterator {
public:
    virtual ~VtStreamOutIterator();
    virtual void Next(std::ostream&) = 0;
};

struct Vt_Reserved;

void VtStreamOutArray(VtStreamOutIterator*, size_t size,
                      const Vt_Reserved*, std::ostream&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_STREAMOUT_H
