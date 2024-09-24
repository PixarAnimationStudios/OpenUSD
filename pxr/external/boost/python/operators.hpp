//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OPERATORS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OPERATORS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/operators.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/def_visitor.hpp"
# include "pxr/external/boost/python/converter/arg_to_python.hpp"
# include "pxr/external/boost/python/detail/operator_id.hpp"
# include "pxr/external/boost/python/detail/not_specified.hpp"
# include "pxr/external/boost/python/back_reference.hpp"
# include "pxr/external/boost/python/detail/mpl2/if.hpp"
# include "pxr/external/boost/python/detail/mpl2/eval_if.hpp"
# include "pxr/external/boost/python/self.hpp"
# include "pxr/external/boost/python/other.hpp"
# include "pxr/external/boost/python/refcount.hpp"
# include "pxr/external/boost/python/detail/unwrap_wrapper.hpp"
# include <string>
# include <sstream>
# include <complex>

namespace PXR_BOOST_NAMESPACE { namespace python { 

namespace detail
{
  template <class T>
  std::string convert_to_string(T const& x)
  {
      std::stringstream s;
      s << x;
      return s.str();
  }

  // This is essentially the old v1 to_python(). It will be eliminated
  // once the public interface for to_python is settled on.
  template <class T>
  PyObject* convert_result(T const& x)
  {
      return converter::arg_to_python<T>(x).release();
  }

  // Operator implementation template declarations. The nested apply
  // declaration here keeps MSVC6 happy.
  template <operator_id> struct operator_l
  {
      template <class L, class R> struct apply;
  };
  
  template <operator_id> struct operator_r
  {
      template <class L, class R> struct apply;
  };

  template <operator_id> struct operator_1
  {
      template <class T> struct apply;
  };

  // MSVC6 doesn't want us to do this sort of inheritance on a nested
  // class template, so we use this layer of indirection to avoid
  // ::template<...> on the nested apply functions below
  template <operator_id id, class L, class R>
  struct operator_l_inner
      : operator_l<id>::template apply<L,R>
  {};
      
  template <operator_id id, class L, class R>
  struct operator_r_inner
      : operator_r<id>::template apply<L,R>
  {};

  template <operator_id id, class T>
  struct operator_1_inner
      : operator_1<id>::template apply<T>
  {};
      
  // Define three different binary_op templates which take care of
  // these cases:
  //    self op self
  //    self op R
  //    L op self
  // 
  // The inner apply metafunction is used to adjust the operator to
  // the class type being defined. Inheritance of the outer class is
  // simply used to provide convenient access to the operation's
  // name().

  // self op self
  template <operator_id id>
  struct binary_op : operator_l<id>
  {
      template <class T>
      struct apply : operator_l_inner<id,T,T>
      {
      };
  };

  // self op R
  template <operator_id id, class R>
  struct binary_op_l : operator_l<id>
  {
      template <class T>
      struct apply : operator_l_inner<id,T,R>
      {
      };
  };

  // L op self
  template <operator_id id, class L>
  struct binary_op_r : operator_r<id>
  {
      template <class T>
      struct apply : operator_r_inner<id,L,T>
      {
      };
  };

  template <operator_id id>
  struct unary_op : operator_1<id>
  {
      template <class T>
      struct apply : operator_1_inner<id,T>
      {
      };
  };

  // This type is what actually gets returned from operators used on
  // self_t
  template <operator_id id, class L = not_specified, class R = not_specified>
  struct operator_
    : def_visitor<operator_<id,L,R> >
  {
   private:
      template <class ClassT>
      void visit(ClassT& cl) const
      {
          typedef typename detail::mpl2::eval_if<
              is_same<L,self_t>
            , detail::mpl2::if_<
                  is_same<R,self_t>
                , binary_op<id>
                , binary_op_l<
                      id
                    , BOOST_DEDUCED_TYPENAME unwrap_other<R>::type
                  >
              >
            , detail::mpl2::if_<
                  is_same<L,not_specified>
                , unary_op<id>
                , binary_op_r<
                      id
                    , BOOST_DEDUCED_TYPENAME unwrap_other<L>::type
                  >
              >
          >::type generator;
      
          cl.def(
              generator::name()
            , &generator::template apply<
                 BOOST_DEDUCED_TYPENAME ClassT::wrapped_type
              >::execute
          );
      }
    
      friend class python::def_visitor_access;
  };
}

# define PXR_BOOST_PYTHON_BINARY_OPERATION(id, rid, expr)       \
namespace detail                                            \
{                                                           \
  template <>                                               \
  struct operator_l<op_##id>                                \
  {                                                         \
      template <class L, class R>                           \
      struct apply                                          \
      {                                                     \
          typedef typename unwrap_wrapper_<L>::type lhs;    \
          typedef typename unwrap_wrapper_<R>::type rhs;    \
          static PyObject* execute(lhs& l, rhs const& r)    \
          {                                                 \
              return detail::convert_result(expr);          \
          }                                                 \
      };                                                    \
      static char const* name() { return "__" #id "__"; }   \
  };                                                        \
                                                            \
  template <>                                               \
  struct operator_r<op_##id>                                \
  {                                                         \
      template <class L, class R>                           \
      struct apply                                          \
      {                                                     \
          typedef typename unwrap_wrapper_<L>::type lhs;    \
          typedef typename unwrap_wrapper_<R>::type rhs;    \
          static PyObject* execute(rhs& r, lhs const& l)    \
          {                                                 \
              return detail::convert_result(expr);          \
          }                                                 \
      };                                                    \
      static char const* name() { return "__" #rid "__"; }  \
  };                                                        \
} 

# define PXR_BOOST_PYTHON_BINARY_OPERATOR(id, rid, op)      \
PXR_BOOST_PYTHON_BINARY_OPERATION(id, rid, l op r)          \
namespace self_ns                                       \
{                                                       \
  template <class L, class R>                           \
  inline detail::operator_<detail::op_##id,L,R>         \
  operator op(L const&, R const&)                       \
  {                                                     \
      return detail::operator_<detail::op_##id,L,R>();  \
  }                                                     \
}
  
PXR_BOOST_PYTHON_BINARY_OPERATOR(add, radd, +)
PXR_BOOST_PYTHON_BINARY_OPERATOR(sub, rsub, -)
PXR_BOOST_PYTHON_BINARY_OPERATOR(mul, rmul, *)
#if PY_VERSION_HEX >= 0x03000000
    PXR_BOOST_PYTHON_BINARY_OPERATOR(truediv, rtruediv, /)
#else
    PXR_BOOST_PYTHON_BINARY_OPERATOR(div, rdiv, /)
#endif
PXR_BOOST_PYTHON_BINARY_OPERATOR(mod, rmod, %)
PXR_BOOST_PYTHON_BINARY_OPERATOR(lshift, rlshift, <<)
PXR_BOOST_PYTHON_BINARY_OPERATOR(rshift, rrshift, >>)
PXR_BOOST_PYTHON_BINARY_OPERATOR(and, rand, &)
PXR_BOOST_PYTHON_BINARY_OPERATOR(xor, rxor, ^)
PXR_BOOST_PYTHON_BINARY_OPERATOR(or, ror, |)
PXR_BOOST_PYTHON_BINARY_OPERATOR(gt, lt, >)
PXR_BOOST_PYTHON_BINARY_OPERATOR(ge, le, >=)
PXR_BOOST_PYTHON_BINARY_OPERATOR(lt, gt, <)
PXR_BOOST_PYTHON_BINARY_OPERATOR(le, ge, <=)
PXR_BOOST_PYTHON_BINARY_OPERATOR(eq, eq, ==)
PXR_BOOST_PYTHON_BINARY_OPERATOR(ne, ne, !=)
# undef PXR_BOOST_PYTHON_BINARY_OPERATOR
    
// pow isn't an operator in C++; handle it specially.
PXR_BOOST_PYTHON_BINARY_OPERATION(pow, rpow, pow(l,r))
# undef PXR_BOOST_PYTHON_BINARY_OPERATION
    
namespace self_ns
{
  template <class L, class R>
  inline detail::operator_<detail::op_pow,L,R>
  pow(L const&, R const&)
  {
      return detail::operator_<detail::op_pow,L,R>();
  }
}


# define PXR_BOOST_PYTHON_INPLACE_OPERATOR(id, op)                  \
namespace detail                                                \
{                                                               \
  template <>                                                   \
  struct operator_l<op_##id>                                    \
  {                                                             \
      template <class L, class R>                               \
      struct apply                                              \
      {                                                         \
          typedef typename unwrap_wrapper_<L>::type lhs;        \
          typedef typename unwrap_wrapper_<R>::type rhs;        \
          static PyObject*                                      \
          execute(back_reference<lhs&> l, rhs const& r)         \
          {                                                     \
              l.get() op r;                                     \
              return python::incref(l.source().ptr());          \
          }                                                     \
      };                                                        \
      static char const* name() { return "__" #id "__"; }       \
  };                                                            \
}                                                               \
namespace self_ns                                               \
{                                                               \
  template <class R>                                            \
  inline detail::operator_<detail::op_##id,self_t,R>            \
  operator op(self_t const&, R const&)                          \
  {                                                             \
      return detail::operator_<detail::op_##id,self_t,R>();     \
  }                                                             \
}

PXR_BOOST_PYTHON_INPLACE_OPERATOR(iadd,+=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(isub,-=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(imul,*=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(idiv,/=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(imod,%=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(ilshift,<<=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(irshift,>>=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(iand,&=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(ixor,^=)
PXR_BOOST_PYTHON_INPLACE_OPERATOR(ior,|=)
    
# define PXR_BOOST_PYTHON_UNARY_OPERATOR(id, op, func_name)         \
namespace detail                                                \
{                                                               \
  template <>                                                   \
  struct operator_1<op_##id>                                    \
  {                                                             \
      template <class T>                                        \
      struct apply                                              \
      {                                                         \
          typedef typename unwrap_wrapper_<T>::type self_t;     \
          static PyObject* execute(self_t& x)                   \
          {                                                     \
              return detail::convert_result(op(x));             \
          }                                                     \
      };                                                        \
      static char const* name() { return "__" #id "__"; }       \
  };                                                            \
}                                                               \
namespace self_ns                                               \
{                                                               \
  inline detail::operator_<detail::op_##id>                     \
  func_name(self_t const&)                                      \
  {                                                             \
      return detail::operator_<detail::op_##id>();              \
  }                                                             \
}
# undef PXR_BOOST_PYTHON_INPLACE_OPERATOR

PXR_BOOST_PYTHON_UNARY_OPERATOR(neg, -, operator-)
PXR_BOOST_PYTHON_UNARY_OPERATOR(pos, +, operator+)
PXR_BOOST_PYTHON_UNARY_OPERATOR(abs, abs, abs)
PXR_BOOST_PYTHON_UNARY_OPERATOR(invert, ~, operator~)
#if PY_VERSION_HEX >= 0x03000000
PXR_BOOST_PYTHON_UNARY_OPERATOR(bool, !!, operator!)
#else
PXR_BOOST_PYTHON_UNARY_OPERATOR(nonzero, !!, operator!)
#endif
PXR_BOOST_PYTHON_UNARY_OPERATOR(int, long, int_)
PXR_BOOST_PYTHON_UNARY_OPERATOR(long, PyLong_FromLong, long_)
PXR_BOOST_PYTHON_UNARY_OPERATOR(float, double, float_)
PXR_BOOST_PYTHON_UNARY_OPERATOR(complex, std::complex<double>, complex_)
PXR_BOOST_PYTHON_UNARY_OPERATOR(str, convert_to_string, str)
PXR_BOOST_PYTHON_UNARY_OPERATOR(repr, convert_to_string, repr)
# undef PXR_BOOST_PYTHON_UNARY_OPERATOR

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OPERATORS_HPP
