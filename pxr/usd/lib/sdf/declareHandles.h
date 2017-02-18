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
#ifndef SDF_DECLAREHANDLES_H
#define SDF_DECLAREHANDLES_H

/// \file sdf/declareHandles.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/weakPtrFacade.h"
#include "pxr/base/tf/declarePtrs.h"

#include <set>
#include <typeinfo>
#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/operators.hpp>
#include <boost/python/pointee.hpp>
#include <boost/type_traits/remove_const.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class SdfLayer;
class SdfSpec;
template <class T> class TfRefPtr;
class Sdf_Identity;

// Sdf_Identities are held via intrusive_ptr so that we can carefully
// manage the ref-count to avoid race conditions -- see
// Sdf_IdentityRegistry::Identify().
typedef boost::intrusive_ptr<Sdf_Identity> Sdf_IdentityRefPtr;

/// \class SdfHandle
///
/// SdfHandle is a smart ptr that calls IsDormant() on the pointed-to
/// object as an extra expiration check so that dormant objects appear to
/// be expired.
///
template <class T>
class SdfHandle : private boost::totally_ordered<SdfHandle<T> > {
public:
    typedef SdfHandle<T> This;
    typedef T SpecType;

    typedef typename boost::remove_const<SpecType>::type NonConstSpecType;
    typedef SdfHandle<NonConstSpecType> NonConstThis;

    SdfHandle() { }
    SdfHandle(TfNullPtrType) { }
    explicit SdfHandle(const Sdf_IdentityRefPtr& id) : _spec(id) { }
    SdfHandle(const SpecType& spec) : _spec(spec) { }

    template <class U>
    SdfHandle(const SdfHandle<U>& x) : _spec(x._spec) { }

    This& operator=(const This& x)
    {
        const_cast<NonConstSpecType&>(_spec) = x._spec;
        return *this;
    }

    template <class U>
    This& operator=(const SdfHandle<U>& x)
    {
        const_cast<NonConstSpecType&>(_spec) = x._spec;
        return *this;
    }

    /// Dereference.  Raises a fatal error if the object is invalid or
    /// dormant.
    SpecType* operator->() const
    {
        if (ARCH_UNLIKELY(_spec.IsDormant())) {
            TF_FATAL_ERROR("Dereferenced an invalid %s",
                           ArchGetDemangled(typeid(SpecType)).c_str());
            return 0;
        }
        return const_cast<SpecType*>(&_spec);
    }

    const SpecType & GetSpec() const
    {
        return _spec;
    }

    void Reset()
    {
        const_cast<SpecType&>(_spec) = SpecType();
    }

#if !defined(doxygen)
    typedef SpecType This::*UnspecifiedBoolType;
#endif

    /// Returns \c true in a boolean context if the object is valid,
    /// \c false otherwise.
    operator UnspecifiedBoolType() const
    {
        return _spec.IsDormant() ? 0 : &This::_spec;
    }
    /// Returns \c false in a boolean context if the object is valid,
    /// \c true otherwise.
    bool operator!() const
    {
        return _spec.IsDormant();
    }

    /// Compares handles for equality.
    template <class U>
    bool operator==(const SdfHandle<U>& other) const
    {
        return _spec == other._spec;
    }

    /// Arranges handles in an arbitrary strict weak ordering.  Note that
    /// this ordering is stable across path changes.
    template <class U>
    bool operator<(const SdfHandle<U>& other) const
    {
        return _spec < other._spec;
    }

    /// Hash.
    friend size_t hash_value(const This &x) {
        return hash_value(x._spec);
    }

private:
    SpecType _spec;

    template <class U> friend class SdfHandle;
};

template <class T>
T*
get_pointer(const SdfHandle<T>& x)
{
    return !x ? 0 : x.operator->();
}

PXR_NAMESPACE_CLOSE_SCOPE

namespace boost {

using PXR_NS::get_pointer;

namespace python {

template <typename T>
struct pointee<PXR_NS::SdfHandle<T> > {
    typedef T type;
};

}

}

PXR_NAMESPACE_OPEN_SCOPE

template <class T>
struct SdfHandleTo {
    typedef SdfHandle<T> Handle;
    typedef SdfHandle<const T> ConstHandle;
    typedef std::vector<Handle> Vector;
    typedef std::vector<ConstHandle> ConstVector;
};

template <>
struct SdfHandleTo<SdfLayer> {
    typedef TfWeakPtr<SdfLayer> Handle;
    typedef TfWeakPtr<const SdfLayer> ConstHandle;
    typedef std::vector<Handle> Vector;
    typedef std::vector<ConstHandle> ConstVector;
};

template <typename T>
typename SdfHandleTo<T>::Handle
SdfCreateHandle(T *p)
{
    return typename SdfHandleTo<T>::Handle(p ? *p : T());
}

template <>
SDF_API SdfHandleTo<SdfLayer>::Handle
SdfCreateHandle(SdfLayer *p);

template <typename T>
typename SdfHandleTo<T>::Handle
SdfCreateNonConstHandle(T const *p)
{
    return SdfCreateHandle(const_cast<T *>(p));
}

struct Sdf_CastAccess {
    template<class DST, class SRC>
    static DST CastSpec(const SRC& spec) {
        return DST(spec);
    }
};

SDF_API bool 
Sdf_CanCastToType(
    const SdfSpec& srcSpec, const std::type_info& destType);

SDF_API bool
Sdf_CanCastToTypeCheckSchema(
    const SdfSpec& srcSpec, const std::type_info& destType);

template <class DST, class SRC>
struct Sdf_SpecTypesAreDirectlyRelated
    : public boost::mpl::or_<boost::is_base_of<DST, SRC>,
                             boost::is_base_of<SRC, DST> >::type
{ };

/// Convert SdfHandle<SRC> \p x to an SdfHandle<DST>. This function
/// behaves similar to a dynamic_cast. If class DST cannot represent 
/// the spec pointed to be \p x, or if the classes DST and SRC are 
/// not directly related to each other in the C++ type hierarchy, 
/// the conversion fails and an invalid handle is returned.
///
/// XXX: The second condition in the above statement is currently untrue.
///      This function will allow casting between spec classes even if
///      they are not directly related. Doing so could lead to schema
///      mismatches and other buggy behavior. 
template <typename DST, typename SRC>
inline
SdfHandle<typename DST::SpecType>
TfDynamic_cast(const SdfHandle<SRC>& x)
{
    typedef typename DST::SpecType Spec;
    typedef SdfHandle<Spec> Handle;

    if (Sdf_CanCastToType(x.GetSpec(), typeid(Spec))) {
        return Handle(Sdf_CastAccess::CastSpec<Spec,SRC>(x.GetSpec()));
    }

    return Handle();
}

template <typename DST, typename SRC>
inline
SdfHandle<typename DST::SpecType>
TfSafeDynamic_cast(const SdfHandle<SRC>& x)
{
    return TfDynamic_cast(x);
}

/// Convert SdfHandle<SRC> \p x to an SdfHandle<DST>. This function
/// behaves similar to a static_cast. No runtime checks are performed
/// to ensure the conversion is valid; it is up to the consumer to
/// ensure this.
template <typename DST, typename SRC>
inline
SdfHandle<typename DST::SpecType>
TfStatic_cast(const SdfHandle<SRC>& x)
{
    typedef typename DST::SpecType Spec;
    typedef SdfHandle<Spec> Handle;
    static_assert(Sdf_SpecTypesAreDirectlyRelated<Spec, SRC>::value,
                  "Spec and SRC must be directly related.");

    return Handle(Sdf_CastAccess::CastSpec<Spec,SRC>(x.GetSpec()));
}

template <typename T>
inline
SdfHandle<typename T::SpecType>
TfConst_cast(const SdfHandle<const typename T::SpecType>& x)
{
    return TfStatic_cast<T>(x);
}

/// Convert SdfHandle<SRC> \p x to an SdfHandle<DST>. This function is
/// similar to TfDynamic_cast, but it allows the SRC and DST spec to be
/// indirectly related, so long as the schema associated with the DST
/// spec type is a subclass of the schema associated with \p x.
template <typename DST, typename SRC>
inline
SdfHandle<typename DST::SpecType>
SdfSpecDynamic_cast(const SdfHandle<SRC>& x)
{
    typedef typename DST::SpecType Spec;
    typedef SdfHandle<Spec> Handle;

    if (Sdf_CanCastToTypeCheckSchema(x.GetSpec(), typeid(Spec))) {
        return Handle(Sdf_CastAccess::CastSpec<Spec,SRC>(x.GetSpec()));
    }

    return Handle();
}

/// Convert SdfHandle<SRC> \p x to an SdfHandle<DST>. This function is
/// similar to TfStatic_cast, but it allows the SRC and DST spec to be
/// indirectly related.
template <typename DST, typename SRC>
inline
SdfHandle<typename DST::SpecType>
SdfSpecStatic_cast(const SdfHandle<SRC>& x)
{
    typedef typename DST::SpecType Spec;
    typedef SdfHandle<Spec> Handle;
    return Handle(Sdf_CastAccess::CastSpec<Spec,SRC>(x.GetSpec()));
}

/// Convert SRC_SPEC to a DST_SPEC.
template <typename DST_SPEC, typename SRC_SPEC>
inline
DST_SPEC
SdfSpecStatic_cast(const SRC_SPEC& x)
{
    return Sdf_CastAccess::CastSpec<DST_SPEC,SRC_SPEC>(x);
}

typedef TfRefPtr<SdfLayer> SdfLayerRefPtr;
typedef std::vector<TfRefPtr<SdfLayer> > SdfLayerRefPtrVector;
typedef std::set<SdfHandleTo<SdfLayer>::Handle> SdfLayerHandleSet;

#define SDF_DECLARE_HANDLES(cls)                                         \
    typedef SdfHandleTo<class cls>::Handle cls##Handle;                  \
    typedef SdfHandleTo<class cls>::ConstHandle cls##ConstHandle;        \
    typedef SdfHandleTo<class cls>::Vector cls##HandleVector;            \
    typedef SdfHandleTo<class cls>::ConstVector cls##ConstHandleVector

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_DECLAREHANDLES_H
