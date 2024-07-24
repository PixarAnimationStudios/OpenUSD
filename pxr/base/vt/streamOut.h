//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_STREAM_OUT_H
#define PXR_BASE_VT_STREAM_OUT_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/functionRef.h"

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

struct Vt_ShapeData;

VT_API void VtStreamOutArray(std::ostream&, const Vt_ShapeData*,
                             TfFunctionRef<void(std::ostream&)>);

#ifdef PXR_PYTHON_SUPPORT_ENABLED
VT_API std::ostream &VtStreamOut(class TfPyObjWrapper const &, std::ostream &);
#endif // PXR_PYTHON_SUPPORT_ENABLED

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_STREAM_OUT_H
