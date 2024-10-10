//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DECORATED_TYPE_ID_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DECORATED_TYPE_ID_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/decorated_type_id.hpp>
#else

# include "pxr/external/boost/python/type_id.hpp"
# include "pxr/external/boost/python/detail/indirect_traits.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

struct decorated_type_info
{
    enum decoration { const_ = 0x1, volatile_ = 0x2, reference = 0x4 };
    
    decorated_type_info(type_info, decoration = decoration());

    inline bool operator<(decorated_type_info const& rhs) const;
    inline bool operator==(decorated_type_info const& rhs) const;

    inline bool operator>(decorated_type_info const& rhs) const;
    inline bool operator<=(decorated_type_info const& rhs) const;
    inline bool operator>=(decorated_type_info const& rhs) const;
    inline bool operator!=(decorated_type_info const& rhs) const;

    friend PXR_BOOST_PYTHON_DECL std::ostream& operator<<(std::ostream&, decorated_type_info const&);

    operator type_info const&() const;
 private: // type
    typedef type_info base_id_t;
    
 private: // data members
    decoration m_decoration;
    base_id_t m_base_type;
};

template <class T>
inline decorated_type_info decorated_type_id(type<T>* = 0)
{
    return decorated_type_info(
        type_id<T>()
        , decorated_type_info::decoration(
            (is_const<T>::value || indirect_traits::is_reference_to_const<T>::value
             ? decorated_type_info::const_ : 0)
            | (is_volatile<T>::value || indirect_traits::is_reference_to_volatile<T>::value
               ? decorated_type_info::volatile_ : 0)
            | (is_reference<T>::value ? decorated_type_info::reference : 0)
            )
        );
}

inline decorated_type_info::decorated_type_info(type_info base_t, decoration decoration)
    : m_decoration(decoration)
    , m_base_type(base_t)
{
}

inline bool decorated_type_info::operator<(decorated_type_info const& rhs) const
{
    return m_decoration < rhs.m_decoration
      || (m_decoration == rhs.m_decoration
          && m_base_type < rhs.m_base_type);
}

inline bool decorated_type_info::operator==(decorated_type_info const& rhs) const
{
    return m_decoration == rhs.m_decoration && m_base_type == rhs.m_base_type;
}

inline bool decorated_type_info::operator>(decorated_type_info const& rhs) const
{
    return rhs < *this;
}

inline bool decorated_type_info::operator<=(decorated_type_info const& rhs) const
{
    return !(rhs < *this);
}

inline bool decorated_type_info::operator>=(decorated_type_info const& rhs) const
{
    return !(*this < rhs);
}

inline bool decorated_type_info::operator!=(decorated_type_info const& rhs) const
{
    return !(*this == rhs);
}

inline decorated_type_info::operator type_info const&() const
{
    return m_base_type;
}

PXR_BOOST_PYTHON_DECL std::ostream& operator<<(std::ostream&, decorated_type_info const&);

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DECORATED_TYPE_ID_HPP
