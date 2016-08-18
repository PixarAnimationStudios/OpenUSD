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
#ifndef BOOST_PP_IS_ITERATING

#ifndef TF_PYNOTICEWRAPPER_H
#define TF_PYNOTICEWRAPPER_H

#ifndef TF_MAX_ARITY
#  define TF_MAX_ARITY 7
#endif // TF_MAX_ARITY

#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/mpl/and.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/preprocessor.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>

#include <map>
#include <string>

struct Tf_PyNoticeObjectGenerator {
    typedef Tf_PyNoticeObjectGenerator This;
    typedef boost::python::object (*MakeObjectFunc)(TfNotice const &);

    // Register the generator for notice type T.
    template <typename T>
    static void Register() {
        // XXX blevin: this stuff should be keyed directly off TfType now
        (*_generators)[typeid(T).name()] = This::_Generate<T>;
    }

    // Produce a boost::python::object for the correct derived type of \a n.
    TF_API static boost::python::object Invoke(TfNotice const &n);

private:

    template <typename T>
    static boost::python::object _Generate(TfNotice const &n) {
        // Python locking is left to the caller.
        return boost::python::object(static_cast<T const &>(n));
    }

    static MakeObjectFunc _Lookup(TfNotice const &n);

    TF_API static TfStaticData<std::map<std::string, MakeObjectFunc> > _generators;

};

struct TfPyNoticeWrapperBase : public TfType::PyPolymorphicBase {
    TF_API virtual ~TfPyNoticeWrapperBase();
    virtual boost::python::handle<> GetNoticePythonObject() const = 0;
};

template <class Notice> 
struct Tf_PyNoticeObjectFinder : public Tf_PyObjectFinderBase {
    virtual ~Tf_PyNoticeObjectFinder() {}
    virtual boost::python::object Find(void const *objPtr) const {
        using namespace boost::python;
        TfPyLock lock;
        TfPyNoticeWrapperBase const *wrapper =
            static_cast<TfPyNoticeWrapperBase const *>(objPtr);
        return wrapper ? object(wrapper->GetNoticePythonObject()) : object();
    }
};

template <typename NoticeType, typename BaseType>
struct TfPyNoticeWrapper : public NoticeType, public TfPyNoticeWrapperBase {
private:
    BOOST_STATIC_ASSERT((boost::mpl::or_
        <boost::is_base_of<TfNotice, NoticeType>,
        boost::is_same<TfNotice, NoticeType> >::value));

    BOOST_STATIC_ASSERT((boost::mpl::or_
        <boost::is_base_of<TfNotice, BaseType>,
        boost::is_same<TfNotice, BaseType> >::value));

    // Base must be a base of Notice, unless Base and Notice are both TfNotice
    // (the root case).
    BOOST_STATIC_ASSERT((boost::mpl::or_
        <boost::is_base_of<BaseType, NoticeType>,
        boost::mpl::and_<boost::is_same<NoticeType, TfNotice>
        , boost::is_same<BaseType, TfNotice> > >::value));

public:

    typedef TfPyNoticeWrapper<NoticeType, BaseType> This;

    // If Notice is really TfNotice, then this is the root of the hierarchy and
    // bases is empty, otherwise bases contains the base class.
    typedef typename boost::mpl::if_<
        boost::is_same<NoticeType, TfNotice>
        , boost::python::bases<>, boost::python::bases<BaseType> >::type Bases;

    typedef boost::python::class_<NoticeType, This, Bases> ClassType;

    static ClassType Wrap(std::string const &name = std::string()) {
        std::string wrappedName = name;
        if (wrappedName.empty()) {
            // Assume they want the last bit of a qualified name.
            wrappedName = TfType::Find<NoticeType>().GetTypeName();
            if (not TfStringGetSuffix(wrappedName, ':').empty())
                wrappedName = TfStringGetSuffix(wrappedName, ':'); 
        }
        Tf_PyNoticeObjectGenerator::Register<NoticeType>();
        Tf_RegisterPythonObjectFinderInternal
        (typeid(TfPyNoticeWrapper),
            new Tf_PyNoticeObjectFinder<TfPyNoticeWrapper>);
        return ClassType(wrappedName.c_str(), boost::python::no_init)
            .def(TfTypePythonClass());
    }

    // Implement the base class's virtual method.
    virtual boost::python::handle<> GetNoticePythonObject() const {
        TfPyLock lock;
        return boost::python::handle<>(boost::python::borrowed(_self));
    }

    // Arbitrary arg constructors.
    TfPyNoticeWrapper(PyObject *self) : NoticeType(), _self(self) {}
#define BOOST_PP_ITERATION_LIMITS (1, TF_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "pxr/base/tf/pyNoticeWrapper.h"
#include BOOST_PP_ITERATE()
    /* comment needed for scons dependency scanner
    #include "pxr/base/tf/pyNoticeWrapper.h"
    */

private:
    PyObject *_self;

};

#define TF_INSTANTIATE_NOTICE_WRAPPER(T, Base) \
TF_REGISTRY_FUNCTION(TfType) \
{ \
    TfType::Define< TfPyNoticeWrapper<T, Base>, \
                    TfType::Bases<Base> >(); \
}

#endif // TF_PYNOTICEWRAPPER_H

#else // BOOST_PP_IS_ITERATING

#define N BOOST_PP_ITERATION()

// Arbitrary argument constructors (with a leading PyObject *) which forward to
// the base Notice class's constructor.

template <BOOST_PP_ENUM_PARAMS(N, typename A)>
TfPyNoticeWrapper(PyObject *self
    BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, A, a)) :
    NoticeType(BOOST_PP_ENUM_PARAMS(N, a)), _self(self) {}

#undef N

#endif // BOOST_PP_IS_ITERATING
